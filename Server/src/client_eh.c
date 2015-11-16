#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "client_eh.h"

struct eh_ctx {
    int fd;
};

static int sendbytes(const char* msg, int fd);
static size_t receivebytes(int fd, char **msg);
//static int disc_user(int epoll_fd, struct epoll_event *es, struct epoll_event *e);

static size_t receivebytes(int fd, char **msg){
    char buff[10000];
    size_t len=0;

    len=read(fd,buff,10000);
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

static void handle_ifconfig(char* iface, int fd){
    struct ifreq ifr = {};
    FILE *proc = NULL;
    char message[1000] = "\0", mac[18] = "\0", buf[256] = "\0", ipv6[40] = "\0";
    iface[strlen(iface)-2] = '\0';

    ifr.ifr_addr.sa_family = AF_INET;

    if(strlen(iface)<IFNAMSIZ-1)
        strncpy(ifr.ifr_name , iface , strlen(iface));//iface - interface name
    else
        strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);//iface - interface name

    strcat(message, "IPv4 Address: ");
    ioctl(fd, SIOCGIFADDR, &ifr);//ipv4
    strcat(message, inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr));
    strcat(message, "\n");

    strcat(message, "Network Mask: ");
    ioctl(fd, SIOCGIFNETMASK, &ifr);//net_mask
    strcat(message, inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr));
    strcat(message, "\n");

    strcat(message, "MAC: ");
    ioctl(fd, SIOCGIFHWADDR, &ifr);//MAC
    sprintf(mac, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
            ifr.ifr_hwaddr.sa_data[0], ifr.ifr_hwaddr.sa_data[1],
            ifr.ifr_hwaddr.sa_data[2], ifr.ifr_hwaddr.sa_data[3],
            ifr.ifr_hwaddr.sa_data[4], ifr.ifr_hwaddr.sa_data[5]);
    strcat(message, mac);

    if((proc = fopen("/proc/net/if_inet6", "r")) == NULL)
        fprintf(stderr, "Could not open /proc/net/if_inet6\n");
    else{
        while(fgets(buf, 256, proc) != NULL){
            if(strstr(buf, iface) != NULL){
                strcat(message, "IPV6: ");
                sprintf(ipv6, "%c%c%c%c:%c%c%c%c:%c%c%c%c:%c%c%c%c:%c%c%c%c:%c%c%c%c:%c%c%c%c:%c%c%c%c\n",
				buf[0], buf[1], buf[2], buf[3],
				buf[4], buf[5], buf[6], buf[7],
				buf[8], buf[9], buf[10], buf[11],
				buf[12], buf[13], buf[14], buf[15],
				buf[16], buf[17], buf[18], buf[19],
				buf[20], buf[21], buf[22], buf[23],
				buf[24], buf[25], buf[26], buf[27],
				buf[28], buf[29], buf[30], buf[31]);
                strcat(message, ipv6);
            }
        }
    }

    sendbytes(message,fd);
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
    char *buff = 0;

    if (e->events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
        return 1;
    else if (e->events & EPOLLIN) {
        msg_len = receivebytes(e->data.fd, &buff);
        if (msg_len > 1) {
            handle_ifconfig(buff,e->data.fd);
            //sendbytes("1.1.Unknown message\n",e->data.fd);
        } else
            return 1;
        if (buff){ free(buff); buff = NULL; }
    }
    return 0;
}

static void destroy_client_eh(event_handler *self){
    close(self->ctx->fd);
    free(self->ctx);
    free(self);
}

event_handler* create_client_eh(int cli_fd){
    event_handler *s_eh = malloc(sizeof(event_handler));
    eh_ctx *ctx = malloc(sizeof(eh_ctx));

    ctx->fd = cli_fd;

    s_eh->ctx = ctx;
    s_eh->get_fd = get_fd;
    s_eh->handle_event = handle_event;
    s_eh->destroy = destroy_client_eh;
    return s_eh;
}
