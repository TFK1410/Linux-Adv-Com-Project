#include "user_list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct ul_ctx{
	size_t size;
	user **users;
};

static user* find_by_fd(user_list *ul, int fd){
	size_t i = 0;
	for(;i<ul->ctx->size;i++){
		if(ul->ctx->users[i] && (ul->ctx->users[i]->fd==fd))
			return ul->ctx->users[i];
	}
	return 0; //return NULL if not found
}

static int add(user_list *ul, user *u){
	size_t i=0;
	if(find_by_fd(ul,u->fd) != 0)
		return 1; //user already exists
	for (;i<ul->ctx->size;++i){
		if(ul->ctx->users[i]==0){
			ul->ctx->users[i] = u;
			//printf("New user added. Name: %s\n",ul->ctx->users[i]->nick);
			return 0;
		}
	}
	return 1; //there is no place in user list
}

static int rm_by_fd(user_list *ul, int fd){
	size_t i=0;

	for(;i<ul->ctx->size;++i){
		if(ul->ctx->users[i] && ul->ctx->users[i]->fd==fd){
			free(ul->ctx->users[i]->nick);
			free(ul->ctx->users[i]);
			ul->ctx->users[i]=0;
			return 0;
		}
	}
	return 1; //user not in list
}

size_t lst(user_list *ul, char **uslist){
	size_t len = 0;
	int i = 0;

	for(;i<ul->ctx->size;i++){
		if(ul->ctx->users[i]!=0)
			len=len+sizeof(ul->ctx->users[i]->nick)+2;
	}

	if(len==0)
		return 0;

	(*uslist)=malloc(len);
	(*uslist)[0]='\0';
	for(i=0;i<ul->ctx->size;i++){
		if(ul->ctx->users[i]!=0)
			sprintf((*uslist),"%s.%s",(*uslist),ul->ctx->users[i]->nick);
	}

	return len;
}

user_list* create_ul(size_t size){
	user_list* res = malloc(sizeof(user_list));
	res->ctx = malloc(sizeof(ul_ctx));
	res->ctx->size = size;
	res->ctx->users = malloc(size*sizeof(user*));
	memset(res->ctx->users, 0, size*sizeof(user*));

	res->add=add;
	res->rm_by_fd=rm_by_fd;
	res->find_by_fd=find_by_fd;
	res->lst=lst;

	return res;
}

void destroy_ul(user_list* ul){
	size_t i = 0;
	for(;i<ul->ctx->size;++i){
		if(ul->ctx->users[i]){
			free(ul->ctx->users[i]->nick);
			free(ul->ctx->users[i]);
			ul->ctx->users[i]=0;
		}
	}
	free(ul->ctx->users);
	free(ul->ctx);
	free(ul);
}
