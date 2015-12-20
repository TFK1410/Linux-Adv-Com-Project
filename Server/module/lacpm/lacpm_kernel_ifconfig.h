#ifndef __lacpm_kernel_ifconfig_h__
#define __lacpm_kernel_ifconfig_h__

#ifndef IFCONFIGURATOR_H
    #include <linux/in.h>
#else
    #include <netinet/in.h>
#endif

#define NETLINK_USER 31
#define LACPM_ERROR -1
#define LACPM_HELLO 0
#define LACPM_SHOW 1
#define LACPM_SETMAC 2
#define LACPM_SETIP 3
#define LACPM_SETMASK 4
#define LACPM_GETNAME 5


struct ifconfig {
    int message_type;
    int index;
    char name[16];
    int flags;
    struct sockaddr mac;
    struct sockaddr_in ipv4;
    struct sockaddr_in ipv4_broadcast;
    struct sockaddr_in ipv4_netmask;
    char ipv6[16];
};

#endif
