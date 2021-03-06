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

static void receive_netlink_message(struct sk_buff * skb);
static void lacpm_ifconfig(struct ifconfig *ifconfig, const struct net_device *dev);
static void lacpm_setmac(struct ifconfig *ifconfig, struct net_device *dev);
static void lacpm_setip(struct ifconfig *ifconfig, struct net_device *dev);
static void lacpm_setmask(struct ifconfig *ifconfig, struct net_device *dev);
static void lacpm_getname(struct ifconfig *ifconfig);
static struct in_ifaddr* lacpm_create_in_ifaddr(struct net_device *dev, struct in_device *in_dev);

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
static void process_message(struct ifconfig * ifconfig, const struct nlmsghdr * header, struct sk_buff *skb) {
    struct net_device * dev = NULL;
    struct sk_buff * skb_out = NULL;

    skb_out = skb_clone(skb, GFP_KERNEL);
    if (!skb_out) {
        printk(KERN_ERR "Failed to clone skb\n");   //if failed it cannot send original message back and blocks the server
        return;                                     //maybe new function to send new empty struct in case of error
    }

    NETLINK_CB(skb_out).portid = 0;
    NETLINK_CB(skb_out).dst_group = 0;

    if (ifconfig->message_type != LACPM_GETNAME){
        dev = dev_get_by_name(&init_net, ifconfig->name);
        if (!dev) {
            ifconfig->message_type = -1;
            lacpm_debug(KERN_DEBUG "Selected interface doesn't exist: %s", ifconfig->name);
            goto out;
        }
    }

    switch (ifconfig->message_type) {
        case LACPM_SHOW:
            //Get interface information into structure
            lacpm_ifconfig(ifconfig, dev);
            break;
        case LACPM_SETMAC:
            //Set MAC from structure to interface
            lacpm_setmac(ifconfig, dev);
            break;
        case LACPM_SETIP:
            //Set IP from structure to interface
            lacpm_setip(ifconfig, dev);
            break;
        case LACPM_SETMASK:
            //Set NETMASK from structure to interface
            lacpm_setmask(ifconfig, dev);
            break;
        case LACPM_GETNAME:
            //Get interface name from index number
            lacpm_getname(ifconfig);
            break;
        default:
            lacpm_debug(KERN_DEBUG "process_message ifconfig->type = %d is not supported", ifconfig->message_type);
            break;
    }

out:
    if (nlmsg_unicast(netlink_socket, skb_out, header->nlmsg_pid) != 0) {
        lacpm_debug(KERN_DEBUG "Failed to send message");
    }
    if (dev != NULL) {
        dev_put(dev);
    }
    //kfree_skb(skb_out); //who frees the memory?
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

    process_message(ifconfig, header, skb);
}

/**
 * lacpm_ifconfig - Called when user wants to get the information about a specific interface
 * @ifconfig - ifconfig structure to be filled with information
 * @dev - device to get the information from
 */
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

    rcu_read_lock();
    in_dev = rcu_dereference(dev->ip_ptr);
    ifap = in_dev->ifa_list;

    if (ifap != NULL) {
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
    } else {
        memset(&ifconfig->ipv4, 0, sizeof(ifconfig->ipv4));
        memset(&ifconfig->ipv4_netmask, 0, sizeof(ifconfig->ipv4_netmask));
        memset(&ifconfig->ipv4_broadcast, 0, sizeof(ifconfig->ipv4_broadcast));
    }

    //IPV6
    in6_dev = rcu_dereference(dev->ip6_ptr);
    if (in6_dev != NULL && !list_empty(&in6_dev->addr_list)) {
        if6ap = list_first_entry(&in6_dev->addr_list, struct inet6_ifaddr, if_list);    //Using kernel list mechanism
        if6addr = &if6ap->addr;
        memcpy(ifconfig->ipv6, &if6addr->in6_u, 16);
    } else {
        memset(ifconfig->ipv6, 0, 16);
    }

    ifconfig->message_type = 0;
    rcu_read_unlock();
}

/**
 * lacpm_setmac - Called when user wants to set a new mac address to the interface
 * @ifconfig - ifconfig structure with filled mac field
 * @dev - device to set the information to
 */
static void lacpm_setmac(struct ifconfig *ifconfig, struct net_device *dev){
    netif_stop_queue(dev);  //IFDOWN
    memcpy(dev->dev_addr, ifconfig->mac.sa_data, 14);
    dev->type = ifconfig->mac.sa_family;
    netif_start_queue(dev); //IFUP
}

/**
 * lacpm_create_in_ifaddr - Call when there is no existing ip address on selected device to start the list and allocate first element
 * @dev - device to add the address structure to
 * @in_dev - dereferenced pointer to in_device struct of selected device
 */
static struct in_ifaddr* lacpm_create_in_ifaddr(struct net_device *dev, struct in_device *in_dev){
    struct in_ifaddr *ifa = NULL;

    ifa = kzalloc(sizeof(struct in_ifaddr), GFP_KERNEL);
    if(!ifa)
        return NULL;
    INIT_HLIST_NODE(&ifa->hash);

    memcpy(ifa->ifa_label, dev->name, IFNAMSIZ);

    ifa->ifa_prefixlen = 32;
    ifa->ifa_mask = htonl(~(1U-1));

    ifa->ifa_dev = in_dev;
    in_dev->ifa_list = ifa;
    return ifa;

}

/**
 * lacpm_setip - Called when user wants to set a new ip address to the interface
 * @ifconfig - ifconfig structure with filled ipv4 field
 * @dev - device to set the information to
 */
static void lacpm_setip(struct ifconfig *ifconfig, struct net_device *dev){
    struct in_device *in_dev;
    struct in_ifaddr *ifap;

    rcu_read_lock();
    rtnl_lock();
    in_dev = rcu_dereference(dev->ip_ptr);
    ifap = in_dev->ifa_list;
    if(!ifap){
        ifap = lacpm_create_in_ifaddr(dev, in_dev);
        if(!ifap){
            ifconfig->message_type = -1;
            rtnl_unlock();
            return;
        }
    }

    if(ifconfig->ipv4.sin_family == AF_INET){
        memcpy(&ifap->ifa_local, &ifconfig->ipv4.sin_addr, sizeof(ifap->ifa_local));
        memcpy(&ifap->ifa_address, &ifconfig->ipv4.sin_addr, sizeof(ifap->ifa_address));
    }
    rtnl_unlock();
    rcu_read_unlock();
}

/**
 * lacpm_setmask - Called when user wants to set a new network mask to the interface
 * @ifconfig - ifconfig structure with filled ipv4_netmask field
 * @dev - device to set the information to
 */
static void lacpm_setmask(struct ifconfig *ifconfig, struct net_device *dev){
    struct in_device *in_dev;
    struct in_ifaddr *ifap;
    __u32 hmask = 0;

    rcu_read_lock();
    rtnl_lock();
    in_dev = rcu_dereference(dev->ip_ptr);
    ifap = in_dev->ifa_list;
    if(!ifap){
        ifap = lacpm_create_in_ifaddr(dev, in_dev);
        if(!ifap){
            ifconfig->message_type = -1;
            rtnl_unlock();
            return;
        }
    }

    if(ifconfig->ipv4_netmask.sin_family == AF_INET){
        memcpy(&ifap->ifa_mask, &ifconfig->ipv4_netmask.sin_addr, sizeof(ifap->ifa_mask));

        ifap->ifa_prefixlen = 0;
        hmask = ntohl(ifap->ifa_mask);
        if (hmask)
            ifap->ifa_prefixlen = 32 - ffz(~hmask);  //ffz = find first zero

        if (dev->flags & IFF_BROADCAST)
            ifap->ifa_broadcast = (ifap->ifa_local | ~ifap->ifa_mask);
    }
    rtnl_unlock();
    rcu_read_unlock();
}

/**
 * lacpm_getname - Called when user wants to get the interface name from its index number
 * @ifconfig - ifconfig structure with filled index field
 */
static void lacpm_getname(struct ifconfig *ifconfig){
    struct net_device * dev = NULL;

    rcu_read_lock();
    dev = dev_get_by_index_rcu(&init_net, ifconfig->index);

    if (dev){
        strncpy(ifconfig->name, dev->name, 16);
        return;
    }
    ifconfig->message_type = -1;
    rcu_read_unlock();
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

    lacpm_debug(KERN_DEBUG "Entering LACMP module");
    netlink_socket = netlink_kernel_create(&init_net, NETLINK_USER, &netlink_kernel_cfg);

    if (!netlink_socket) {
        goto fail;
    }

    return 0;

fail:
    return 1;
}


module_init(lacpm_init_mod);
module_exit(lacpm_cleanup);
