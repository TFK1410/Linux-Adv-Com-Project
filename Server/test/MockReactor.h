extern "C" {
#include "../src/reactor.h"
}
#include <gmock/gmock.h>

class MockReactor {
public:
    MockReactor();
    struct reactor *getStruct();

    MOCK_METHOD1(add_eh, bool(event_handler *));
    MOCK_METHOD1(rm_eh, void(event_handler *));
    MOCK_METHOD0(event_loop, void());
    MOCK_METHOD0(destroy, void());
private:
    static bool c2cpp_add_eh(struct reactor *self, event_handler *eh);
    static void c2cpp_rm_eh(struct reactor *self, event_handler *eh);
    static void c2cpp_event_loop(struct reactor *self);
    static void c2cpp_destroy(struct reactor *self);

    struct reactor mockedObject;
};
