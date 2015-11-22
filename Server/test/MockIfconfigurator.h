extern "C" {
#include "../src/ifconfigurator.h"
}
#include <gmock/gmock.h>

class MockIfconfigurator {
public:
    MockIfconfigurator();
    struct ifconfigurator *getStruct();

    MOCK_METHOD2(for_each_interface, void(void (*)(const char *, void *), void *));
    MOCK_METHOD2(get_if_config, bool(const char *iface, struct ifconfig *));
    MOCK_METHOD2(set_ip, bool(const char *iface, struct in_addr *));
    MOCK_METHOD2(set_net_mask, bool(const char *iface, struct in_addr *));
    MOCK_METHOD2(set_mac, bool(const char *, struct sockaddr *));
    MOCK_METHOD0(destroy, void());

private:

    static void c2cpp_for_each_interface(struct ifconfigurator *self, void (*callback)(const char *iface, void *ctx), void *ctx);
    static bool c2cpp_get_if_config(struct ifconfigurator *self, const char *iface, struct ifconfig *out_config);
    static bool c2cpp_set_ip(struct ifconfigurator *self, const char *iface, struct in_addr *new_addr);
    static bool c2cpp_set_net_mask(struct ifconfigurator *self, const char *iface, struct in_addr *new_net_mask);
    static bool c2cpp_set_mac(struct ifconfigurator *self, const char *iface, struct sockaddr *new_mac);
    static void c2cpp_destroy(struct ifconfigurator *self);

    struct ifconfigurator mockedObject;
};
