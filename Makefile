MAJOR := 1
MINOR := 0

CFLAGS := -O3 -g -Wall -Werror -Isrc
CC := gcc

NAME := metrist-libcurl-agent
GIT_TAG := $(shell git rev-parse --short HEAD)
SRC := $(shell pwd)/src
TEST := $(shell pwd)/test
BUILD := $(shell pwd)/build
VERSION := $(MAJOR).$(MINOR).$(GIT_TAG)
LIB_NAME := $(NAME).so.$(VERSION)
LIB := $(BUILD)/$(LIB_NAME)

.PHONY: all

all: $(LIB) test

$(BUILD):
	mkdir -p $(BUILD)

$(LIB): $(SRC)/*.[ch] | $(BUILD)
	$(CC) $(CFLAGS) -fPIC -shared -Wl,-z,defs -Wl,--as-needed -Wl,-soname,$(LIB_NAME) $(filter *.c, $^) -o $@ -lc

clean:
	$(RM) -rf $(BUILD)

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
