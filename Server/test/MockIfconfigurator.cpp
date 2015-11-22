#include "MockIfconfigurator.h"

MockIfconfigurator::MockIfconfigurator() {
    mockedObject.ctx = (struct ifconfigurator_ctx *) this;
    mockedObject.for_each_interface = c2cpp_for_each_interface;
    mockedObject.get_if_config = c2cpp_get_if_config;
    mockedObject.set_ip = c2cpp_set_ip;
    mockedObject.set_net_mask = c2cpp_set_net_mask;
    mockedObject.set_mac = c2cpp_set_mac;
    mockedObject.destroy = c2cpp_destroy;
}

struct ifconfigurator *MockIfconfigurator::getStruct()
{
    return &mockedObject;
}

void MockIfconfigurator::c2cpp_for_each_interface(struct ifconfigurator *self, void (*callback)(const char *iface, void *ctx), void *ctx)
{
    MockIfconfigurator *thiz = (MockIfconfigurator *) self->ctx;
    thiz->for_each_interface(callback, ctx);
}
bool MockIfconfigurator::c2cpp_get_if_config(struct ifconfigurator *self, const char *iface, struct ifconfig *out_config)
{
    MockIfconfigurator *thiz = (MockIfconfigurator *) self->ctx;
    return thiz->get_if_config(iface, out_config);
}
bool MockIfconfigurator::c2cpp_set_ip(struct ifconfigurator *self, const char *iface, struct in_addr *new_addr)
{
    MockIfconfigurator *thiz = (MockIfconfigurator *) self->ctx;
    return thiz->set_ip(iface, new_addr);
}
bool MockIfconfigurator::c2cpp_set_net_mask(struct ifconfigurator *self, const char *iface, struct in_addr *new_net_mask)
{
    MockIfconfigurator *thiz = (MockIfconfigurator *) self->ctx;
    return thiz->set_net_mask(iface, new_net_mask);
}
bool MockIfconfigurator::c2cpp_set_mac(struct ifconfigurator *self, const char *iface, struct sockaddr *new_mac)
{
    MockIfconfigurator *thiz = (MockIfconfigurator *) self->ctx;
    return thiz->set_mac(iface, new_mac);
}
void MockIfconfigurator::c2cpp_destroy(struct ifconfigurator *self)
{
    MockIfconfigurator *thiz = (MockIfconfigurator *) self->ctx;
    thiz->destroy();
}
