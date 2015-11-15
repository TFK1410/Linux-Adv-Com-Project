#BUILDROOT_PATH=/home/xubuntu/buildroot
#HOST_PATH=$(BUILDROOT_PATH)/output/host
#CC=$(HOST_PATH)/usr/bin/i686-buildroot-linux-uclibc-gcc
#SYSROOT=$(HOST_PATH)/usr/i686-buildroot-linux-uclibc/sysroot

CFLAGS = -Wall -g
LDFLAGS =
SOURCES = ../src/main.c ../src/client_eh.c ../src/eh_list.c ../src/reactor.c ../src/server_eh.c 
NAME = server

OBJECTS = $(SOURCES:.c=.o)

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(NAME)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS)
	rm -f $(NAME)
