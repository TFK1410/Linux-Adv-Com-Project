#ifndef MOCK_EVENT_HANDLER_H
#define MOCK_EVENT_HANDLER_H
extern "C" {
#include "../src/event_handler.h"
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

class MockEventHandler {
public:
    MockEventHandler();
    struct event_handler *getStruct();

    MOCK_METHOD0(get_fd, int());
    MOCK_METHOD1(handle_event, int(uint32_t events));
    MOCK_METHOD0(destroy, void());

private:
    static int c2cpp_get_fd(event_handler *self);
    static int c2cpp_handle_event(event_handler *self, uint32_t events);
    static void c2cpp_destroy(event_handler *self);
event_handler mockedObject;
};
#endif
