#include "reactor.h"

#include <stdlib.h>
#include <errno.h>

struct reactor_ctx{
    size_t size;
    size_t registered_handlers;
    int epoll_fd;
};

static bool add_eh(reactor *self, event_handler *eh) {
    struct epoll_event e = {
        .events = EPOLLIN,
        .data = {
            .ptr = eh
        }
    };

    /* Enforce max events limit */
    if (self->ctx->registered_handlers >= self->ctx->size) {
        fprintf(stderr, "Dropping event handler due to limit\n");
        eh->destroy(eh);
        return false;
    }
    self->ctx->registered_handlers++;

    if (epoll_ctl(self->ctx->epoll_fd, EPOLL_CTL_ADD, eh->get_fd(eh), &e) < 0) {
        fprintf(stderr, "Cannot add socket to epoll\n");
        eh->destroy(eh);
        return false;
    }
    return true;
}

static void rm_eh(reactor *self, event_handler *eh){
    struct epoll_event e = {};
    if (epoll_ctl(self->ctx->epoll_fd, EPOLL_CTL_DEL, eh->get_fd(eh), &e) < 0) {
        fprintf(stderr, "Cannot remove socket from epoll\n");
        // Probably bogus rm_eh call; but generally not fatal error
    }

    // Increase available spaces count up
    self->ctx->registered_handlers--;
}

static void event_loop(reactor *self){
    int i=0;
    struct epoll_event *events = calloc(self->ctx->size, sizeof(struct epoll_event));

    for (;;) {
        i = epoll_wait(self->ctx->epoll_fd, events, self->ctx->size, -1);
        if (i < 0) {
            if (errno == -EAGAIN) {
                continue;
            }
            fprintf(stderr, "Cannot wait for events\n");
            break;
        }

        for(--i;i>-1;--i){
            event_handler *eh = events[i].data.ptr;
            if (eh->handle_event(eh, events[i].events)) {
                self->rm_eh(self, eh);
                eh->destroy(eh);
            }
        }
    }

    free(events);
}

static void destroy_reactor(reactor *r) {
    close(r->ctx->epoll_fd);
    free(r->ctx);
    free(r);
}

reactor* create_reactor(int size){
    int epoll_fd = epoll_create(size);
    if (epoll_fd < 0) {
        fprintf(stderr, "Cannot create epoll\n");
        return NULL;
    }
    reactor *r = malloc(sizeof(reactor));
    reactor_ctx *ctx = malloc(sizeof(reactor_ctx));

    r->ctx = ctx;
    r->ctx->epoll_fd = epoll_fd;
    r->ctx->size = size;
    r->ctx->registered_handlers = 0;

    r->add_eh = add_eh;
    r->rm_eh = rm_eh;
    r->event_loop = event_loop;
    r->destroy = destroy_reactor;

    return r;
}

