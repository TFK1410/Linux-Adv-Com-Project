#ifndef CLIENT_EH_H
#define CLIENT_EH_H

#include "event_handler.h"
#include "ifconfigurator.h"

event_handler* create_client_eh(int cli_fd, ifconfigurator *ifc);
void create_userlist(size_t size);

#endif
