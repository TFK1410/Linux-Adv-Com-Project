#ifndef MOCK_REACTOR_H
#define MOCK_REACTOR_H
extern "C" {
#include "../src/reactor.h"
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

class MockReactor {
public:
    MockReactor();
    struct reactor *getStruct();

    MOCK_METHOD1(add_eh, bool(event_handler *eh));
    MOCK_METHOD1(rm_eh, void(event_handler *eh));
    MOCK_METHOD0(event_loop, void());
    MOCK_METHOD0(destroy, void());

private:
    static bool c2cpp_add_eh(reactor *self, event_handler *eh);
    static void c2cpp_rm_eh(reactor *self, event_handler *eh);
    static void c2cpp_event_loop(reactor *self);
    static void c2cpp_destroy(reactor *self);
reactor mockedObject;
};
#endif
