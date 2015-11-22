#ifndef REACTOR_H
#define REACTOR_H

#include "event_handler.h"
#include <stdbool.h>

typedef struct reactor_ctx reactor_ctx;

typedef struct reactor {
    reactor_ctx *ctx;
    /**
     * Add an event handler to reactor, after this call reactor owns
     * eh and is responsible for destroying it
     *
     * Returns false and destroys eh on failure
     */
    bool (* add_eh)(struct reactor *self, event_handler *eh);

    /**
     * Explicitly unregister handler from reactor;
     * the object won't be destroyed
     */
    void (* rm_eh)(struct reactor *self, event_handler *eh);

    void (* event_loop)(struct reactor *self);

    void (* destroy)(struct reactor *r);
} reactor;

reactor* create_reactor(int size);

#endif
