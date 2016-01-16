#include <stdlib.h>
#include <libconfig.h>
#include "server_eh.h"
#include "ifconfigurator.h"
#include "signal.h"

static int load_cfg(char* cfg_name, int* port, int* size){
    config_t cfg;
    config_setting_t *setting = NULL;

    config_init(&cfg);

    if (!config_read_file(&cfg, cfg_name))
    {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    setting = config_lookup(&cfg, "cfg");

    if (setting != NULL) {
        if (!config_setting_lookup_int(setting, "port", port)) {
            (*port) = 5555;
            fprintf(stderr, "Port number not specified. Using default 5555.\n");
        }

        if (!config_setting_lookup_int(setting, "users", size)) {
            (*size) = 10;
            fprintf(stderr, "Max number of users not specified. Using default 10.\n");
        }
    }

    config_destroy(&cfg);
    return 0;
}

int main(int argc, const char *argv[])
{
    int port = 0, size = 0;
    if(geteuid()!=0){
        fprintf(stderr, "Server requires admin privileges. Run as root.\n");
        return 1;
    }

    if(load_cfg("server.conf", &port, &size)){
        fprintf(stderr, "Error when reading config file. Shutting down now.\n");
        return 1;
    }

    // Ignore SIGPIPE
    {
        struct sigaction action = {
            .sa_handler = SIG_IGN
        };
        sigaction(SIGPIPE, &action, NULL);
    }

    // Create ifconfigurator
    ifconfigurator *ifc = create_netlink_ifconfigurator();
    if (ifc != NULL) {
        printf("Using our driver for configuring interfaces\n");
    } else {
        printf("Falling back to ioctl for configuring interfaces\n");
        ifc = create_ioctl_ifconfigurator();
    }


    reactor *r = create_reactor(size);
    event_handler *s_eh = create_server_eh(r,port,ifc, create_client_eh);
    if (s_eh != NULL){
        r->add_eh(r,s_eh);
        r->event_loop(r);
    }

    return 0;
    ////////////
}
