#ifndef SERVER_EH_H
#define SERVER_EH_H

#include "reactor.h"
#include "event_handler.h"
#include "client_eh.h"

event_handler* create_server_eh(reactor *r, int port, ifconfigurator *ifc, event_handler* (*create_client_eh)(int cli_fd, ifconfigurator *ifc));

#endif
