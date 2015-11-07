#include <stdlib.h>
#include "server_eh.h"

#define BUFF_SIZE 1024
/*
user_list *users;
int sendbytes(const char* msg, int fd);
size_t receivebytes(int fd, char **msg);
int disc_user(int epoll_fd, struct epoll_event *es, struct epoll_event *e);

size_t receivebytes(int fd, char **msg){
	size_t len=0;

	if(read(fd,&len,sizeof(size_t))<sizeof(size_t)){
		printf("Read error. Disconnecting user.\n");
		return 0;
	}
	(*msg)=malloc((len+1)*sizeof(char));
	if(read(fd,(*msg),len)<len){
		printf("Read error. Disconnecting user.\n");
		return 0;
	}
	(*msg)[len]='\0';

	return len;
}

int sendbytes(const char* msg, int fd){
	size_t len=strlen(msg);

	if(len>0){
		if(write(fd, &len, sizeof(size_t))<sizeof(size_t)){
			printf("Write error. Disconnecting user.\n");
			return 1;
		}
		write(fd,msg,len);
	}

return 0;
}

int handle_userlist(int cnt, int fd){
	size_t len = 0;
	char *msg = 0, *lst = 0;

	len = users->lst(users, &lst) + 2;
	if(len==2){
		msg = malloc(3*sizeof(char));
		sprintf(msg,"7.");
	} else {
		msg = malloc(len*sizeof(char));
		sprintf(msg,"7%s",lst);
	}

	sendbytes(msg,fd);

	if(msg){ free(msg); msg = NULL;}
	if(lst){ free(lst); lst = NULL;}
	return 0;
}

int handle_login(char* msg, size_t len, int fd){
	user *usr = malloc(sizeof(user));

	usr->fd=fd;
    strsep(&msg,".");
	usr->nick=malloc(strlen(msg)*sizeof(char)+1);
	strcpy(usr->nick,msg);
	if(users->add(users,usr)!=0){
		sendbytes("1.1.Nie mozna Cie zalogowac.",fd);
		free(usr->nick);
		free(usr);
		return 1;
	}
	sendbytes("1.0", fd);

	return 0;
}

int disc_user(int epoll_fd, struct epoll_event *es, struct epoll_event *e){
	users->rm_by_fd(users,es->data.fd);
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, es->data.fd, e);
	close(es->data.fd);
	es->data.fd = 0;
	return 0;
}
*/
int main(int argc, const char *argv[])
{
	/*
    int i = 0;
    char *buff = 0;
    size_t msg_len = 0;

    int srv_fd = -1;
    int cli_fd = -1;
    int epoll_fd = -1;
*/

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
	r->add_eh(r,s_eh);
	r->event_loop(r);

	return 0;
	////////////
/*
    //set socket config
    struct sockaddr_in srv_addr;
    struct sockaddr_in cli_addr;
    socklen_t cli_addr_len = sizeof(struct sockaddr_in);
    struct epoll_event e, es[10];

    users = create_ul(cnt);
    memset(&srv_addr, 0, sizeof(srv_addr));
    memset(&cli_addr, 0, sizeof(cli_addr));
    memset(&e, 0, sizeof(e));

    srv_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (srv_fd < 0) {
        printf("Cannot create socket\n");
        return 1;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port); //TODO: read port from command line
    if (bind(srv_fd, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0) {
        printf("Cannot bind socket\n");
        close(srv_fd);
        return 1;
    }

    if (listen(srv_fd, 1) < 0) {
        printf("Cannot listen\n");
        close(srv_fd);
        return 1;
    }

    epoll_fd = epoll_create(2);
    if (epoll_fd < 0) {
        printf("Cannot create epoll\n");
        close(srv_fd);
        return 1;
    }

    e.events = EPOLLIN;
    e.data.fd = srv_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, srv_fd, &e) < 0) {
        printf("Cannot add server socket to epoll\n");
        close(epoll_fd);
        close(srv_fd);
        return 1;
    }

    e.data.fd = 1;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, 1, &e) < 0) {
        printf("Cannot add stdin socket to epoll\n");
        close(epoll_fd);
        close(srv_fd);
        return 1;
    }

	printf("Server is up and running! Type \"e...\" or \"E...\" to exit.\n");

    for(;;) {
        i = epoll_wait(epoll_fd, es, 10, -1);
        if (i < 0) {
            printf("Cannot wait for events\n");
            close(epoll_fd);
            close(srv_fd);
            return 1;
        }

        for (--i; i > -1; --i) {
            if (es[i].data.fd == srv_fd) {	//connect accept
                cli_fd = accept(srv_fd, (struct sockaddr*) &cli_addr, &cli_addr_len);
                if (cli_fd < 0) {
                    printf("Cannot accept client\n");
                    close(epoll_fd);
                    close(srv_fd);
                    return 1;
                }

                e.data.fd = cli_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cli_fd, &e) < 0) {
                    printf("Cannot accept client\n");
                    close(epoll_fd);
                    close(srv_fd);
                    return 1;
                }
            } else if (es[i].data.fd == 1) {	//server exit
				char ch=fgetc(stdin);
				while(fgetc(stdin)!='\n');
					if(ch=='E' || ch=='e'){
						close(epoll_fd);
						close(srv_fd);
						if (buff) free(buff);
						destroy_ul(users);
						return 0;
				}
            } else {				//communication
				if (es[i].events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
					disc_user(epoll_fd, &es[i], &e);
				else if (es[i].events & EPOLLIN) {
                    msg_len = receivebytes(es[i].data.fd, &buff);
                    if (msg_len > 1) {
                        if(buff[0]=='2')
							handle_login(buff,msg_len,es[i].data.fd);
						else if(buff[0]=='6')
							handle_userlist(cnt, es[i].data.fd);
						else
							sendbytes("1.1.Unknown message",es[i].data.fd);
					} else
						disc_user(epoll_fd, &es[i], &e);
					if (buff){ free(buff); buff = NULL;}
                }
            }
        }
    }

	return 0;
	*/
}

