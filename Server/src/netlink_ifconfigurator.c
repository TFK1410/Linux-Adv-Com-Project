#include "ifconfigurator.h"
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <net/if.h>
#include <unistd.h>


struct ifconfigurator_ctx {
    int fd;
};

static bool bind_netlink(struct ifconfigurator_ctx *ctx){
    struct sockaddr_nl src_addr = {};

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();

    return bind(ctx->fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) == 0;
}

static unsigned char * set_nl_header(unsigned char * data, int size)
{
    struct nlmsghdr * nlh = (struct nlmsghdr *)data;

    memset(nlh, 0, NLMSG_SPACE(size));
    nlh->nlmsg_len = NLMSG_SPACE(size);
    nlh->nlmsg_pid = getpid();
    return data + NLMSG_HDRLEN;
}

static int netlink_transfer(int sock_fd, struct ifconfig *ifc){
    int result, size = sizeof(struct ifconfig);
    unsigned char * data_with_hdr = malloc(NLMSG_SPACE(size));
    unsigned char * data_point = set_nl_header(data_with_hdr, size);
    struct sockaddr_nl addr;

    //SEND
    memset(data_with_hdr, 0, NLMSG_SPACE(size));
    data_point = set_nl_header(data_with_hdr, size);

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = 0; /* For Linux Kernel */
    addr.nl_groups = 0; /* unicast */

    memcpy(data_point, ifc, size);
    result = sendto(sock_fd, data_with_hdr, NLMSG_SPACE(size), 0, (struct sockaddr *)&addr, sizeof addr);
    memset(data_with_hdr, 0, NLMSG_SPACE(size));

    //RECEIVE
    result = recv(sock_fd, data_with_hdr, NLMSG_SPACE(size), 0);
    if (result <= 0 || result < NLMSG_SPACE(size)) {
        return 0;
    }
    memmove(data_with_hdr, data_with_hdr + NLMSG_HDRLEN, result - NLMSG_HDRLEN); //sanity check
    memcpy(ifc, data_with_hdr, sizeof(struct ifconfig));
    free(data_with_hdr);
    return ifc->message_type != -1;
}

static void ifconfigurator_for_each_interface(struct ifconfigurator *self, void (*callback)(const char *iface, void *ctx), void *ctx)
{
    struct ifconfig ifc = {};
    int i = 1;
    ifc.mac.sa_family = AF_INET;
    for (; ; ++i) {
        ifc.message_type = LACPM_GETNAME;
        ifc.index = i;
        if(!netlink_transfer(self->ctx->fd, &ifc))
            return;
        callback(ifc.name, ctx);
    }
}

static void prepare_if_request(struct ifconfig *ifc, const char *iface) {
    memset(ifc, 0, sizeof(struct ifconfig));
    strncpy(ifc->name, iface, IFNAMSIZ-1); // iface - interface name
}

static bool ifconfigurator_get_if_config(struct ifconfigurator *self, const char *iface, struct ifconfig *out_config)
{
    prepare_if_request(out_config, iface);
    out_config->message_type = LACPM_SHOW;
    if(netlink_transfer(self->ctx->fd, out_config) && out_config){
        return true;
    }

    return false;
}

static bool ifconfigurator_set_ip(struct ifconfigurator *self, const char *iface, struct in_addr *new_addr)
{
    struct ifconfig ifc;
    prepare_if_request(&ifc, iface);
    ifc.ipv4.sin_addr = *new_addr;
    ifc.message_type = LACPM_SETIP;
    ifc.ipv4.sin_family = AF_INET;
    return netlink_transfer(self->ctx->fd, &ifc) != 0;
}

static bool ifconfigurator_set_net_mask(struct ifconfigurator *self, const char *iface, struct in_addr *new_net_mask)
{
    struct ifconfig ifc;
    prepare_if_request(&ifc, iface);
    ifc.ipv4_netmask.sin_addr = *new_net_mask;
    ifc.message_type = LACPM_SETMASK;
    ifc.ipv4_netmask.sin_family = AF_INET;
    return netlink_transfer(self->ctx->fd, &ifc) != 0;
}

static bool ifconfigurator_set_mac(struct ifconfigurator *self, const char *iface, struct sockaddr *new_mac)
{
    struct ifconfig ifc;
    prepare_if_request(&ifc, iface);
    ifc.mac = *new_mac;
    ifc.message_type = LACPM_SETMAC;
    ifc.mac.sa_family = AF_INET;
    return netlink_transfer(self->ctx->fd, &ifc) != 0;
}

static void ifconfigurator_destroy(struct ifconfigurator *self)
{
    close(self->ctx->fd);
    free(self->ctx);
    free(self);
}

ifconfigurator *create_netlink_ifconfigurator()
{
    ifconfigurator *self = malloc(sizeof(ifconfigurator));
    ifconfigurator_ctx *ctx = malloc(sizeof(ifconfigurator_ctx));

    ctx->fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);

    if (ctx->fd < 0) {
        free(ctx);
        free(self);
        printf("Netlink socket creation failed\n");
        return NULL;
    }

    if (!bind_netlink(ctx)){
        free(ctx);
        free(self);
        printf("Netlink socket binding failed\n");
        return NULL;
    }

    self->ctx = ctx;
    self->for_each_interface = ifconfigurator_for_each_interface;
    self->get_if_config = ifconfigurator_get_if_config;
    self->set_ip = ifconfigurator_set_ip;
    self->set_net_mask = ifconfigurator_set_net_mask;
    self->set_mac = ifconfigurator_set_mac;
    self->destroy = ifconfigurator_destroy;

    return self;
}
