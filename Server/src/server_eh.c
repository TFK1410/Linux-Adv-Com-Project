#include "server_eh.h"
#include "ifconfigurator.h"
#include <stdlib.h>
#include <stdio.h>

struct eh_ctx {
    int fd;
    reactor *r;
    ifconfigurator *ifconfigurator;
    event_handler* (*create_client_eh)(int cli_fd, ifconfigurator *ifc);
};

static int get_fd(event_handler *self){
    return self->ctx->fd;
}

static int handle_event(event_handler *self, uint32_t events) {
    event_handler *cli_eh = 0;
    int cli_fd = -1;

    cli_fd = accept(self->ctx->fd, NULL, NULL);
    if(cli_fd<0){
        fprintf(stderr, "Cannot accept client\n");
        exit(1);
    }

    cli_eh = self->ctx->create_client_eh(cli_fd, self->ctx->ifconfigurator);
    self->ctx->r->add_eh(self->ctx->r,cli_eh);
    return 0;
}

static void destroy_server_eh(event_handler *self){
    close(self->ctx->fd);
    free(self->ctx);
    free(self);
}

event_handler* create_server_eh(reactor *r, int port, ifconfigurator *ifc, event_handler* (*create_client_eh)(int cli_fd, ifconfigurator *ifc)){
    event_handler *s_eh = malloc(sizeof(event_handler));
    eh_ctx *ctx = malloc(sizeof(eh_ctx));

    //set socket config
    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));

    ctx->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (ctx->fd < 0) {
        fprintf(stderr, "Cannot create socket\n");
        return NULL;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    if (bind(ctx->fd, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0) {
        fprintf(stderr, "Cannot bind socket\n");
        close(ctx->fd);
        free(s_eh);
        free(ctx);
        return NULL;
    }

    if (listen(ctx->fd, 1) < 0) {
        fprintf(stderr, "Cannot listen\n");
        close(ctx->fd);
        free(s_eh);
        free(ctx);
        return NULL;
    }

    ctx->r=r;
    ctx->ifconfigurator = ifc;
    ctx->create_client_eh = create_client_eh;

    s_eh->ctx = ctx;
    s_eh->get_fd = get_fd;
    s_eh->handle_event = handle_event;
    s_eh->destroy = destroy_server_eh;

    printf("Server is up and running!\n");

    return s_eh;
}
