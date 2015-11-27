#include "MockEventHandler.h"

/*         ^                                     */
/*        / \           AUTO GENERATED FILE      */
/*       /   \          DO NOT EDIT MANUALLY     */
/*      /  |  \                                  */
/*     /   |   \                                 */
/*    /         \       Run generatemocks.py     */
/*   /     o     \     to regenerate this file   */
/*  /_____________\                              */

MockEventHandler::MockEventHandler() {
    mockedObject.ctx = (eh_ctx *) this;
    mockedObject.get_fd = c2cpp_get_fd;
    mockedObject.handle_event = c2cpp_handle_event;
    mockedObject.destroy = c2cpp_destroy;
}

struct event_handler *MockEventHandler::getStruct()
{
    return &mockedObject;
}

int MockEventHandler::c2cpp_get_fd(event_handler *self) {
    MockEventHandler *thiz = (MockEventHandler *) self->ctx;
    return thiz->get_fd();
}
int MockEventHandler::c2cpp_handle_event(event_handler *self, uint32_t events) {
    MockEventHandler *thiz = (MockEventHandler *) self->ctx;
    return thiz->handle_event(events);
}
void MockEventHandler::c2cpp_destroy(event_handler *self) {
    MockEventHandler *thiz = (MockEventHandler *) self->ctx;
    thiz->destroy();
}
