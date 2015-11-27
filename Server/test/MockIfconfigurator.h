#ifndef MOCK_IFCONFIGURATOR_H
#define MOCK_IFCONFIGURATOR_H
extern "C" {
#include "../src/ifconfigurator.h"
}
#include <gmock/gmock.h>

/*         ^                                     */
/*        / \           AUTO GENERATED FILE      */
/*       /   \          DO NOT EDIT MANUALLY     */
/*      /  |  \                                  */
/*     /   |   \                                 */
/*    /         \       Run generatemocks.py     */
/*   /     o     \     to regenerate this file   */
/*  /_____________\                              */

class MockIfconfigurator {
public:
    MockIfconfigurator();
    struct ifconfigurator *getStruct();

    MOCK_METHOD2(for_each_interface, void(void (*callback)(const char *iface, void *ctx), void *ctx));
    MOCK_METHOD2(get_if_config, bool(const char *iface, struct ifconfig *out_config));
    MOCK_METHOD2(set_ip, bool(const char *iface, struct in_addr *new_addr));
    MOCK_METHOD2(set_net_mask, bool(const char *iface, struct in_addr *new_net_mask));
    MOCK_METHOD2(set_mac, bool(const char *iface, struct sockaddr *new_mac));
    MOCK_METHOD0(destroy, void());

private:
    static void c2cpp_for_each_interface(ifconfigurator *self, void (*callback)(const char *iface, void *ctx), void *ctx);
    static bool c2cpp_get_if_config(ifconfigurator *self, const char *iface, struct ifconfig *out_config);
    static bool c2cpp_set_ip(ifconfigurator *self, const char *iface, struct in_addr *new_addr);
    static bool c2cpp_set_net_mask(ifconfigurator *self, const char *iface, struct in_addr *new_net_mask);
    static bool c2cpp_set_mac(ifconfigurator *self, const char *iface, struct sockaddr *new_mac);
    static void c2cpp_destroy(ifconfigurator *self);
ifconfigurator mockedObject;
};
#endif
