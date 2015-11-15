#include <stdlib.h>
#include "server_eh.h"

int main(int argc, const char *argv[])
{
    if(argc<2 || argc>3){
        printf("Wrong argument count.\n");
        printf("run using task1 <user_numb> [<port>]\n");
        return 1;
    }
    size_t size = atoi(argv[1])+1;
    int port = 5555;
    if(argc==3)
        port = atoi(argv[2]);

    ////////////
    reactor *r = create_reactor(size);
    event_handler *s_eh = create_server_eh(r,port,size);
    if (s_eh != NULL){
        r->add_eh(r,s_eh);
        r->event_loop(r);
    }

    return 0;
    ////////////
}
