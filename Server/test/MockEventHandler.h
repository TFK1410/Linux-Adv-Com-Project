extern "C" {
#include "../src/event_handler.h"
}
#include <gmock/gmock.h>

class MockEventHandler {
public:
    MockEventHandler();
    struct event_handler *getStruct();

    MOCK_METHOD0(get_fd, int());
    MOCK_METHOD1(handle_event, int(uint32_t));
    MOCK_METHOD0(destroy, void());

private:
    static int c2cpp_get_fd(struct event_handler *self);
    static int c2cpp_handle_event(struct event_handler *self, uint32_t events);
    static void c2cpp_destroy(struct event_handler *self);

    struct event_handler mockedObject;
};
