#include "ifconfigurator.h"
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>


struct ifconfigurator_ctx {
    int fd;
};

static void ifconfigurator_for_each_interface(struct ifconfigurator *self, void (*callback)(const char *iface, void *ctx), void *ctx)
{
    struct ifreq ifr = {};
    int i = 1;
    ifr.ifr_addr.sa_family = AF_INET;
    for (; ; ++i) {
        ifr.ifr_ifindex = i;
        if(ioctl(self->ctx->fd, SIOCGIFNAME, &ifr) == -1)
            return;
        callback(ifr.ifr_name, ctx);
    }
}

static void prepare_if_request(struct ifreq *ifr, const char *iface) {
    memset(ifr, 0, sizeof(struct ifreq));
    ifr->ifr_addr.sa_family = AF_INET;
    strncpy(ifr->ifr_name, iface, IFNAMSIZ-1); // iface - interface name
}

static bool ifconfigurator_get_if_config(struct ifconfigurator *self, const char *iface, struct ifconfig *out_config)
{
    struct ifreq ifr;
    FILE *proc = NULL;

    prepare_if_request(&ifr, iface);

    // Get flags
    if(ioctl(self->ctx->fd, SIOCGIFFLAGS, &ifr) == -1){
        return false;
    }
    out_config->flags = ifr.ifr_flags;

    // Get MAC
    memset(&ifr.ifr_hwaddr, 0, sizeof(ifr.ifr_hwaddr)); // Clear, so failed ioctl won't lead to junk value
    ioctl(self->ctx->fd, SIOCGIFHWADDR, &ifr);
    memcpy(&out_config->mac, &ifr.ifr_hwaddr, sizeof(out_config->mac));

    // Get IPv4
    memset(&ifr.ifr_addr, 0, sizeof(ifr.ifr_addr));
    ioctl(self->ctx->fd, SIOCGIFADDR, &ifr);
    memcpy(&out_config->ipv4, &ifr.ifr_addr, sizeof(out_config->ipv4));

    // Get broadcast addr
    memset(&ifr.ifr_broadaddr, 0, sizeof(ifr.ifr_broadaddr));
    ioctl(self->ctx->fd, SIOCGIFBRDADDR, &ifr);
    memcpy(&out_config->ipv4_broadcast, &ifr.ifr_broadaddr, sizeof(out_config->ipv4_broadcast));

    // Get net mask
    memset(&ifr.ifr_netmask, 0, sizeof(ifr.ifr_netmask));
    ioctl(self->ctx->fd, SIOCGIFNETMASK, &ifr);
    memcpy(&out_config->ipv4_netmask, &ifr.ifr_netmask, sizeof(out_config->ipv4_netmask));

    // Get IPv6
    memset(out_config->ipv6, 0, sizeof(out_config->ipv6));
    if((proc = fopen("/proc/net/if_inet6", "r")) == NULL) {
        fprintf(stderr, "Could not open /proc/net/if_inet6\n");
    } else {
        char line[256] = "\0";
        char *if_in_line;
        while(fgets(line, 256, proc) != NULL) {
            utils_trim_string(line);
            if_in_line = strrchr(line, ' ');
            if (if_in_line == NULL) {
                continue;
            }
            if (strcmp(if_in_line + 1, iface) == 0) {
                int i;
                for (i = 0; i < 16; i++) {
                    out_config->ipv6[i] = utils_uchar_from_hex(line[i * 2]) << 4 | utils_uchar_from_hex(line[i * 2 + 1]);
                }
                break;
            }
        }
        fclose(proc);
    }

    return true;
}

bool ifconfigurator_set_ip(struct ifconfigurator *self, const char *iface, struct in_addr *new_addr)
{
    struct ifreq ifr;
    prepare_if_request(&ifr, iface);
    ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr = *new_addr;
    return ioctl(self->ctx->fd, SIOCSIFADDR, &ifr) == 0;
}

bool ifconfigurator_set_net_mask(struct ifconfigurator *self, const char *iface, struct in_addr *new_net_mask)
{
    struct ifreq ifr;
    prepare_if_request(&ifr, iface);
    ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr = *new_net_mask;
    return ioctl(self->ctx->fd, SIOCSIFNETMASK, &ifr) == 0;
}

bool ifconfigurator_set_mac(struct ifconfigurator *self, const char *iface, struct sockaddr *new_mac)
{
    struct ifreq ifr;
    prepare_if_request(&ifr, iface);
    ifr.ifr_hwaddr = *new_mac;
    return ioctl(self->ctx->fd, SIOCSIFHWADDR, &ifr) == 0;
}

void ifconfigurator_destroy(struct ifconfigurator *self)
{
    close(self->ctx->fd);
    free(self->ctx);
    free(self);
}

ifconfigurator *create_ifconfigurator()
{
    ifconfigurator *self = malloc(sizeof(ifconfigurator));
    ifconfigurator_ctx *ctx = malloc(sizeof(ifconfigurator_ctx));

    ctx->fd = socket(AF_INET, SOCK_STREAM, 0);

    self->ctx = ctx;
    self->for_each_interface = ifconfigurator_for_each_interface;
    self->get_if_config = ifconfigurator_get_if_config;
    self->set_ip = ifconfigurator_set_ip;
    self->set_net_mask = ifconfigurator_set_net_mask;
    self->set_mac = ifconfigurator_set_mac;
    self->destroy = ifconfigurator_destroy;

    return self;
}
