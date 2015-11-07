#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "client_eh.h"

struct eh_ctx {
	int fd;
};

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

int handle_userlist(int fd){
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

/*
int disc_user(int epoll_fd, struct epoll_event *es, struct epoll_event *e){
	users->rm_by_fd(users,es->data.fd);
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, es->data.fd, e);
	close(es->data.fd);
	es->data.fd = 0;
	return 0;
}
*/

static int get_fd(event_handler *self){
	return self->ctx->fd;
}

static int handle_event(event_handler *self, const struct epoll_event *e){
	size_t msg_len = -1;
	char *buff = 0;

	if (e->events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP))
		return 1;
	else if (e->events & EPOLLIN) {
        msg_len = receivebytes(e->data.fd, &buff);
        if (msg_len > 1) {
			if(buff[0]=='2')
				handle_login(buff,msg_len,e->data.fd);
			else if(buff[0]=='6')
				handle_userlist(e->data.fd);
			else
				sendbytes("1.1.Unknown message",e->data.fd);
		} else
			return 1;
		if (buff){ free(buff); buff = NULL; }
	}
	return 0;
}

static void destroy_client_eh(event_handler *self){
	users->rm_by_fd(users,self->ctx->fd);
	close(self->ctx->fd);
	free(self->ctx);
	free(self);
}

void create_userlist(size_t size){
	users=create_ul(size);
}

event_handler* create_client_eh(int cli_fd){
	event_handler *s_eh = malloc(sizeof(event_handler));
	eh_ctx *ctx = malloc(sizeof(eh_ctx));

	ctx->fd = cli_fd;

	s_eh->ctx = ctx;
	s_eh->get_fd = get_fd;
	s_eh->handle_event = handle_event;
	s_eh->destroy = destroy_client_eh;
	return s_eh;
}
