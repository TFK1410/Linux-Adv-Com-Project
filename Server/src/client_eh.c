#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <regex.h>
#include <inttypes.h>
#include "client_eh.h"
#include "ifconfigurator.h"
#include "utils.h"

struct eh_ctx {
    int fd;
    regex_t mac;
    regex_t ipm;
    ifconfigurator *ifconfigurator;
};

static int sendbytes(const char* msg, int fd);
static size_t receivebytes(int fd, char **msg);
//static int disc_user(int epoll_fd, struct epoll_event *es, struct epoll_event *e);

static size_t receivebytes(int fd, char **msg){
    char buff[10000];
    size_t len=0;

    len=read(fd,buff,10000);
    if(len < 1) return 0;
    if(len>10000) lseek(fd,0,SEEK_END);

    (*msg)=malloc((len+1)*sizeof(char));
    strncpy((*msg),buff,len);
    (*msg)[len]='\0';

    return len;
}

static int sendbytes(const char* msg, int fd){
    size_t len=strlen(msg);

    if(len>0)
        write(fd,msg,len);

    return 0;
}

static void handle_ifconfig(event_handler *self, const char* iface) {
    char message[1000] = "\0";
    char mac[20] = "\0";
    char ipv4[20] = "\0";
    char netmask[20] = "\0";
    char ipv6[40] = "\0";
    struct ifconfig config;
    bool success;


    success = self->ctx->ifconfigurator->get_if_config(self->ctx->ifconfigurator, iface, &config);
    if (!success) {
        sprintf(message, "No device with name %s found!\n", iface);
        sendbytes(message, self->ctx->fd);
        return;
    }

    /* IPv4 */
    strncpy(ipv4, inet_ntoa(config.ipv4.sin_addr), sizeof(ipv4));

    /* Net mask */
    strncpy(netmask, inet_ntoa(config.ipv4_netmask.sin_addr), sizeof(netmask));

    /* MAC */
    snprintf(
        mac, sizeof(mac),
        "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
        config.mac.sa_data[0],
        config.mac.sa_data[1],
        config.mac.sa_data[2],
        config.mac.sa_data[3],
        config.mac.sa_data[4],
        config.mac.sa_data[5]
    );

    /* IPv6 */
    snprintf(
        ipv6, sizeof(ipv6),
        //"%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:"
        "%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx",
        config.ipv6[ 0],
        config.ipv6[ 1],
        config.ipv6[ 2],
        config.ipv6[ 3],
        config.ipv6[ 4],
        config.ipv6[ 5],
        config.ipv6[ 6],
        config.ipv6[ 7],
        config.ipv6[ 8],
        config.ipv6[ 9],
        config.ipv6[ 10],
        config.ipv6[ 11],
        config.ipv6[ 12],
        config.ipv6[ 13],
        config.ipv6[ 14],
        config.ipv6[ 15]
    );

    /* Whole message */
    snprintf(
        message, sizeof(message),
        "Interface name: %s\n"
            "Status: %s\n"
            "IPv4 Address: %s\n"
            "Network Mask: %s\n"
            "MAC: %s\n"
            "IPV6: %s\n",
        iface,
        (config.flags & IFF_UP) ? "UP" : "DOWN",
        ipv4,
        netmask,
        mac,
        ipv6
    );
    sendbytes(message, self->ctx->fd);
}


static void handle_ifconfig_setip(event_handler *self, char* iface, char* ip){
    //struct ifreq ifr = {};
    struct sockaddr_in addr;
    char message[1000] = "\0", ipmask[20] = "\0", *word = NULL;
    u_int mask = 0, bitmask = 0;

    if(ip == NULL || regexec(&self->ctx->ipm, ip, 0, NULL, 0) == REG_NOMATCH){
        sendbytes("Invalid ip address and mask format or not given at all (a.b.c.d/mask)\n", self->ctx->fd);
        return;
    }

    utils_trim_string(iface);

    /*
    TODO: verify if device exists or just fail?
    if(device_exists) {            //search for device
        sprintf(message, "No device with name %s found!\n", iface);
        sendbytes(message, self->ctx->fd);
        return;
    }
    */

    word = strtok(ip,"/");
    inet_pton(AF_INET, word, &addr.sin_addr); //convert ip to binary
    if (!self->ctx->ifconfigurator->set_ip(self->ctx->ifconfigurator, iface, &addr.sin_addr)){             //set ip address
        sprintf(message, "Error setting IP address!\n");
        sendbytes(message, self->ctx->fd);
        return;
    }

    /* TODO: clean this conversion code (binary -> text -> binary again) */
    mask = atoi(strtok(NULL,""));
    bitmask = 0xFFFFFFFF & ~((1<<(32-mask))-1);
    snprintf(ipmask, sizeof(ipmask), "%d.%d.%d.%d", (bitmask & 0xFF000000)>>24, (bitmask & 0x00FF0000)>>16, (bitmask & 0x0000FF00)>>8, bitmask & 0x000000FF);

    inet_pton(AF_INET, ipmask, &addr.sin_addr);    //convert mask to binary
    if (!self->ctx->ifconfigurator->set_net_mask(self->ctx->ifconfigurator, iface, &addr.sin_addr)) {           //set network mask
        sprintf(message, "Error setting network mask!\n");
        sendbytes(message, self->ctx->fd);
        return;
    }

    snprintf(message, sizeof(message), "Successfully changed ip of the interface %s to %s and mask %s\n", iface, ip, ipmask);
    sendbytes(message, self->ctx->fd);
}

static void handle_ifconfig_setmac(event_handler *self, char* iface, char* mac){
    struct sockaddr hwaddr = {};
    char message[1000] = "\0";

    if(mac == NULL || regexec(&self->ctx->mac, mac, 0, NULL, 0)){
        sendbytes("Invalid mac address format or not given at all (12:34:56:78:90:ab)\n", self->ctx->fd);
        return;
    }

    utils_trim_string(iface);

    /*
    TODO: verify if device exists or just fail?
    if(device_exists) {            //search for device
        sprintf(message, "No device with name %s found!\n", iface);
        sendbytes(message, self->ctx->fd);
        return;
    }
    */

    hwaddr.sa_family = ARPHRD_ETHER;
    sscanf(mac, "%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%" SCNx8 ":%" SCNx8,
        &hwaddr.sa_data[0],
        &hwaddr.sa_data[1],
        &hwaddr.sa_data[2],
        &hwaddr.sa_data[3],
        &hwaddr.sa_data[4],
        &hwaddr.sa_data[5]
    );

    if (!self->ctx->ifconfigurator->set_mac(self->ctx->ifconfigurator, iface, &hwaddr)){             //set ip address
        sendbytes("Error setting MAC address!\n", self->ctx->fd);
        return;
    }

    sprintf(message, "Successfully changed mac address of the interface %s to %s\n", iface, mac);
    sendbytes(message, self->ctx->fd);
}

static void list_interfaces_callback(const char *iface, void *ctx)
{
    handle_ifconfig((event_handler *) ctx, iface);
}

static void handle_ifconfig_all(event_handler *self){
    self->ctx->ifconfigurator->for_each_interface(self->ctx->ifconfigurator, list_interfaces_callback, self);
}

/*
int disc_user(int epoll_fd, struct epoll_event *es, struct epoll_event *e){
    users->rm_by_fd(users,es->data.fd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, es->data.fd, e);
    close(es->data.fd);
    es->data.fd = 0;
    return 0;
}
*/

static int get_fd(event_handler *self){
    return self->ctx->fd;
}

static int handle_event(event_handler *self, const struct epoll_event *e){
    size_t msg_len = -1;
    char *buff = 0, *word = 0, *dev = 0;

    if (e->events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
        return 1;
    else if (e->events & EPOLLIN) {
        msg_len = receivebytes(self->ctx->fd, &buff);
        while(msg_len > 2 && !isalnum(buff[msg_len-1])){
            buff[msg_len-1]='\0';
            --msg_len;
        }
        if (msg_len > 1) {
            word = strtok(buff," ");
            if(strcmp("show",word)==0){
                word = strtok(NULL," ");
                if(!word)
                    handle_ifconfig_all(self);
                else{
                    /* TODO: Deduplicate these lines */
                    utils_trim_string(word);
                    handle_ifconfig(self, word);
                    while((word = strtok(NULL," ")) != NULL) {
                        utils_trim_string(word);
                        handle_ifconfig(self, word);
                    }
                }
            }
            else if(strcmp("setip",word)==0){
                dev = strtok(NULL," ");
                if(!dev)
                    sendbytes("Write setip <interface> <a.b.c.d/mask>\n", self->ctx->fd);
                else{
                    word = strtok(NULL," ");
                    handle_ifconfig_setip(self,dev,word);
                }
            }
            else if(strcmp("setmac",word)==0){
                dev = strtok(NULL," ");
                if(!dev)
                    sendbytes("Write setmac <interface> <aa:bb:cc:dd:ee:ff>\n",self->ctx->fd);
                else{
                    word = strtok(NULL," ");
                    handle_ifconfig_setmac(self,dev,word);
                }
            }
            else
                sendbytes("Unknown message\n",self->ctx->fd);
        } else
            return 1;
        if (buff){ free(buff); buff = NULL; }
    }
    return 0;
}

static void destroy_client_eh(event_handler *self){
    close(self->ctx->fd);
    regfree(&self->ctx->ipm);
    regfree(&self->ctx->mac);
    free(self->ctx);
    free(self);
}

event_handler* create_client_eh(int cli_fd, ifconfigurator *ifc){
    event_handler *s_eh = malloc(sizeof(event_handler));
    eh_ctx *ctx = malloc(sizeof(eh_ctx));

    if(regcomp(&ctx->ipm, "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\/([0-9]|[1-2][0-9]|3[0-2])$", REG_EXTENDED)){
        free(s_eh);
        free(ctx);
        return NULL;
    }

    if(regcomp(&ctx->mac, "^([0-f]{2}:){5}[0-f]{2}$", REG_EXTENDED)){
        regfree(&ctx->ipm);
        free(s_eh);
        free(ctx);
        return NULL;
    }

    ctx->ifconfigurator = ifc;
    ctx->fd = cli_fd;

    s_eh->ctx = ctx;
    s_eh->get_fd = get_fd;
    s_eh->handle_event = handle_event;
    s_eh->destroy = destroy_client_eh;
    return s_eh;
}
