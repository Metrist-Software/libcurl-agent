CFLAGS := -O3 -g -Wall -Werror -Isrc
CC := gcc
MAJOR := 0
MINOR := 1
NAME := metrist-libcurl-agent
GIT_TAG := $(shell git rev-parse --short HEAD)
SRC := $(shell pwd)/src
TEST := $(shell pwd)/test
BUILD := $(shell pwd)/build

VERSION := $(MAJOR).$(MINOR).$(GIT_TAG)

.PHONY: all lib clean

all: dirs lib test

dirs: build
	mkdir -p build

lib: $(NAME).so.$(VERSION)

lib$(NAME).so.$(VERSION): $(NAME).c linked_list.c
	$(CC) $(CFLAGS) -fPIC -shared -Wl,-z,defs -Wl,--as-needed -Wl,-soname,lib$(NAME).so.$(MAJOR) $^ -o $@ -lc

clean:
	$(RM) *.o *.so* linked_list_test curl_test

test:
	$(MAKE) CFLAGS="-DDEBUG -g" clean lib linked_list_test curl_test
	./linked_list_test
	LD_AUDIT=./lib$(NAME).so.$(VERSION) ./curl_test

linked_list_test: linked_list_test.c linked_list.c
	$(CC) $(CFLAGS) $^ -o $@

curl_test: curl_test.c
	$(CC) $(CFLAGS) $^ -o $@ -lcurl

perf: perf.c linked_list.c
	$(CC) $(CFLAGS) $^ -o $@
