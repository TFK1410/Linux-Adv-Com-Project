#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/netlink.h>
#include <lacpm/lacpm_kernel_ifconfig.h>
#include <linux/inetdevice.h>
#include <net/if_inet6.h>
#include <linux/in6.h>
#include <linux/list.h>
#include <linux/if.h>

#define lacpm_debug(...) printk(__VA_ARGS__)

MODULE_LICENSE("GPL");


static struct sock * netlink_socket = NULL;
static int lacpm_server_pid = 0;

static void receive_netlink_message(struct sk_buff * skb);
static void lacpm_hello(int pid);
static void lacpm_ifconfig(struct ifconfig *ifconfig, const struct net_device *dev);
static void lacpm_setmac(struct ifconfig *ifconfig, struct net_device *dev);
static void lacpm_setip_mask(struct ifconfig *ifconfig, struct net_device *dev);
static void lacpm_getname(struct ifconfig *ifconfig);

static struct netlink_kernel_cfg netlink_kernel_cfg = {
    .input = receive_netlink_message
};

/**
 * process_message - Interprets message received through NetLink socket
 * @ifconfig: structure containing data to fill in or used to set selected interface atributes
 * @header: NetLink message header
 *
 * Interprets message received through NetLink. Performs action
 * specified by ctl->type
 */
static void process_message(struct ifconfig * ifconfig, const struct nlmsghdr * header) {
    struct net_device * dev = NULL;
    struct sk_buff * skb_out = NULL;
    struct nlmsghdr * newheader = NULL;

    skb_out = nlmsg_new(sizeof(struct ifconfig), GFP_KERNEL);
    if (!skb_out) {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    newheader = nlmsg_put(skb_out, 0, 0, 0, sizeof(struct ifconfig), 0);

    //NETLINK_CB(skb_out).portid = 0;
    NETLINK_CB(skb_out).dst_group = 0;

    if (!lacpm_server_pid) {
        ifconfig->message_type = -1;
        lacpm_debug(KERN_DEBUG "No lacpm_server attached to module");
        goto out;
    }

    if (ifconfig->message_type != LACPM_GETNAME){
        dev = dev_get_by_name(&init_net, ifconfig->name);
        if (!dev) {
            ifconfig->message_type = -1;
            lacpm_debug(KERN_DEBUG "Selected interface doesn't exist: %s", ifconfig->name);
            goto out;
        }
    }

    switch (ifconfig->message_type) {
        case LACPM_HELLO:
            //Handshake with server
            lacpm_hello(header->nlmsg_pid);
            break;
        case LACPM_SHOW:
            //Get interface information into structure
            lacpm_ifconfig(ifconfig, dev);
            break;
        case LACPM_SETMAC:
            //Set MAC from structure to interface
            lacpm_setmac(ifconfig, dev);
            break;
        case LACPM_SETIP_SETMASK:
            //Set IP and NETMASK from structure to interface
            lacpm_setip_mask(ifconfig, dev);
            break;
        case LACPM_GETNAME:
            //Get interface name from index number
            lacpm_getname(ifconfig);
        default:
            lacpm_debug(KERN_DEBUG "process_message ifconfig->type = %d is not supported", ifconfig->message_type);
            break;
    }

out:
    memcpy(nlmsg_data(newheader), &ifconfig, sizeof(struct ifconfig));
    if (nlmsg_unicast(netlink_socket, skb_out, lacpm_server_pid) != 0) {
        lacpm_debug(KERN_DEBUG "Failed to send message");
    }
}

/**
 * receive_netlink_message - NetLink receive callback
 * @skb: sk_buff representing NetLink message
 *
 * Receives NetLink message, analyzes ctl header and performs
 * action specified by ctl->type
 */
static void receive_netlink_message(struct sk_buff * skb) {
    struct nlmsghdr * header;
    struct ifconfig * ifconfig;

    header = (struct nlmsghdr *) skb->data;
    ifconfig = nlmsg_data(header);

    process_message(ifconfig, header);
    printk(KERN_DEBUG "MESSAGE PROCESSED");
    //kfree_skb(skb);
}

/**
 * lacpm_hello - Called when user space server wants to connect to the module
 */
static void lacpm_hello(int pid) {
    lacpm_server_pid = pid;
    lacpm_debug("hello %d", pid);
}


static void lacpm_ifconfig(struct ifconfig * ifconfig, const struct net_device * dev){
    struct sockaddr_in * sin = NULL;

    struct in_device *in_dev;
    struct in_ifaddr *ifap;

    struct inet6_dev *in6_dev;
    struct inet6_ifaddr *if6ap;
    struct in6_addr *if6addr;

    //STATUS and INDEX
    ifconfig->flags = dev->flags;
    ifconfig->index = dev->ifindex;

    //MAC
    if (!dev->addr_len)
        memset(ifconfig->mac.sa_data, 0, sizeof(ifconfig->mac.sa_data));
    else
        memcpy(ifconfig->mac.sa_data, dev->dev_addr, sizeof(ifconfig->mac.sa_data));
    ifconfig->mac.sa_family = dev->type;

    in_dev = rcu_dereference(dev->ip_ptr);
    ifap = in_dev->ifa_list;

    //IPV4
    sin = (struct sockaddr_in *)&ifconfig->ipv4;
    sin->sin_family = AF_INET;
    sin->sin_port = 0;
    sin->sin_addr.s_addr = ifap->ifa_local;

    //NETMASK
    sin = (struct sockaddr_in *)&ifconfig->ipv4_netmask;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = ifap->ifa_mask;

    //BROADCAST
    sin = (struct sockaddr_in *)&ifconfig->ipv4_broadcast;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = ifap->ifa_broadcast;

    //IPV6
    in6_dev = rcu_dereference(dev->ip6_ptr);
    if6ap = list_first_entry_or_null(&in6_dev->addr_list, struct inet6_ifaddr, if_list);    //Using kernel list mechanism
    if (if6ap) {
        if6addr = &if6ap->addr;
        memcpy(ifconfig->ipv6, &if6addr->in6_u, 16);
    } else
        memset(ifconfig->ipv6, 0, 16);

    ifconfig->message_type = 0;
}

static void lacpm_setmac(struct ifconfig *ifconfig, struct net_device *dev){
    netif_stop_queue(dev);  //IFDOWN
    memcpy(dev->dev_addr, ifconfig->mac.sa_data, 14);
    dev->type = ifconfig->mac.sa_family;
    netif_start_queue(dev); //IFUP
}

static void lacpm_setip_mask(struct ifconfig *ifconfig, struct net_device *dev){
    struct in_device *in_dev;
    struct in_ifaddr *ifap;

    in_dev = rcu_dereference(dev->ip_ptr);
    ifap = in_dev->ifa_list;

    if(ifconfig->ipv4.sin_family == AF_INET){
        memcpy(&ifap->ifa_local, &ifconfig->ipv4.sin_addr, sizeof(ifap->ifa_local));
        memcpy(&ifap->ifa_address, &ifconfig->ipv4.sin_addr, sizeof(ifap->ifa_address));
    }

    if(ifconfig->ipv4_netmask.sin_family == AF_INET){
        memcpy(&ifap->ifa_mask, &ifconfig->ipv4_netmask.sin_addr, sizeof(ifap->ifa_mask));
    }
}

static void lacpm_getname(struct ifconfig *ifconfig){
    struct net_device * dev = NULL;

    rcu_read_lock();
    dev = dev_get_by_index_rcu(&init_net, ifconfig->index);
    rcu_read_unlock();

    if (dev){
        strncpy(ifconfig->name, dev->name, 16);
        return;
    }
    ifconfig->message_type = -1;
}

/**
 * lacpm_cleanup - Called when module is deinitialized (rmmod)
 */
static void lacpm_cleanup(void) {
    if (netlink_socket) {
        netlink_kernel_release(netlink_socket);
    }
    lacpm_debug(KERN_DEBUG "Exiting LACMP module");
}


/**
 * lacpm_init_mod - Called when module is initialized (insmod)
 */
static int __init lacpm_init_mod(void) {
    struct net_device *dev;
    struct in_device *in_dev;
    struct in_ifaddr *ifap;

    struct inet6_dev *in6_dev;
    struct inet6_ifaddr *if6ap;
    struct in6_addr *if6addr;


    lacpm_debug(KERN_DEBUG "Entering LACMP module");
    netlink_socket = netlink_kernel_create(&init_net, NETLINK_USER, &netlink_kernel_cfg);

    if (!netlink_socket) {
        goto fail;
    }

    dev = dev_get_by_name(&init_net, "eth0");

    if (!dev) {
        lacpm_debug(KERN_DEBUG "lkms_add_netdevices dev_get_by_name failed to find device with name eth0");
        goto fail;
    }
    //STATUS
    if(dev->flags & IFF_UP)
        lacpm_debug(KERN_DEBUG "UP\n");
    else
        lacpm_debug(KERN_DEBUG "DOWN\n");

    //MAC
    lacpm_debug(KERN_DEBUG "%pM\n", dev->dev_addr);

    //IPV4 MASK BROADCAST
    in_dev = rcu_dereference(dev->ip_ptr);
    ifap = in_dev->ifa_list;
    if(ifap) {
        lacpm_debug(KERN_DEBUG "%pI4\n", &ifap->ifa_local);
        lacpm_debug(KERN_DEBUG "%pI4\n", &ifap->ifa_mask);
        lacpm_debug(KERN_DEBUG "%pI4\n", &ifap->ifa_broadcast);
    }

    //IPV6
    in6_dev = rcu_dereference(dev->ip6_ptr);
    if6ap = list_first_entry_or_null(&in6_dev->addr_list, struct inet6_ifaddr, if_list);    //Using kernel list mechanism
    if (if6ap){
        if6addr = &if6ap->addr;
        lacpm_debug(KERN_DEBUG "%pI6\n", &if6addr->in6_u);
    }

    //SET IPV4 MASK MAC
    //ifap->ifa_address++;
    //ifap->ifa_local = ifap->ifa_address;
    //ifap->ifa_mask--;
    lacpm_debug(KERN_DEBUG "%pM\n", dev->dev_addr);
    lacpm_debug(KERN_DEBUG "After: %pI4\n", &ifap->ifa_local);
    lacpm_debug(KERN_DEBUG "After: %pI4\n", &ifap->ifa_mask);

    return 0;

fail:
    return 1;
}


module_init(lacpm_init_mod);
module_exit(lacpm_cleanup);
