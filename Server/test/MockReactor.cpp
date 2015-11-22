#include "MockReactor.h"

MockReactor::MockReactor() {
    mockedObject.ctx = (struct reactor_ctx *) this;
    mockedObject.add_eh = c2cpp_add_eh;
    mockedObject.rm_eh = c2cpp_rm_eh;
    mockedObject.event_loop = c2cpp_event_loop;
    mockedObject.destroy = c2cpp_destroy;
}

struct reactor *MockReactor::getStruct()
{
    return &mockedObject;
}

bool MockReactor::c2cpp_add_eh(struct reactor *self, event_handler *eh)
{
    MockReactor *thiz = (MockReactor *) self->ctx;
    return thiz->add_eh(eh);
}
void MockReactor::c2cpp_rm_eh(struct reactor *self, event_handler *eh)
{
    MockReactor *thiz = (MockReactor *) self->ctx;
    thiz->rm_eh(eh);
}
void MockReactor::c2cpp_event_loop(struct reactor *self)
{
    MockReactor *thiz = (MockReactor *) self->ctx;
    thiz->event_loop();
}
void MockReactor::c2cpp_destroy(struct reactor *self)
{
    MockReactor *thiz = (MockReactor *) self->ctx;
    thiz->destroy();
}
