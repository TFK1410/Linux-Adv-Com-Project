#ifndef IFCONFIGURATOR_H
#define IFCONFIGURATOR_H

#include <stdbool.h>
#include <netinet/in.h>

struct ifconfig {
    int flags;
    struct sockaddr mac;
    struct sockaddr_in ipv4;
    struct sockaddr_in ipv4_broadcast;
    struct sockaddr_in ipv4_netmask;
    char ipv6[16];
};

typedef struct ifconfigurator_ctx ifconfigurator_ctx;

typedef struct ifconfigurator {
    ifconfigurator_ctx *ctx;
    void (* for_each_interface)(struct ifconfigurator *self, void (*callback)(const char *iface, void *ctx), void *ctx);
    bool (* get_if_config)(struct ifconfigurator *self, const char *iface, struct ifconfig *out_config);
    bool (* set_ip)(struct ifconfigurator *self, const char *iface, struct in_addr *new_addr);
    bool (* set_net_mask)(struct ifconfigurator *self, const char *iface, struct in_addr *new_net_mask);
    bool (* set_mac)(struct ifconfigurator *self, const char *iface, struct sockaddr *new_mac);
    void (* destroy)(struct ifconfigurator *self);
} ifconfigurator;

ifconfigurator *create_ifconfigurator();

#endif
