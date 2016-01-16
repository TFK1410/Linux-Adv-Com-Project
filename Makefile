# Common Makefile for building both Server and Module

target:
	make -C Server/build server
	make -C Module target

host:
	make -C Server/build server_host
	make -C Module host

test:
	make -C Server/build test

clean:
	make -C Server/build clean
	make -C Module clean


.PHONY: target host test clean
