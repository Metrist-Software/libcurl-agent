MAJOR := 1
MINOR := 0

NAME := metrist-libcurl-agent
GIT_TAG := $(shell git rev-parse --short HEAD)
SRC := $(shell pwd)/src
TEST := $(shell pwd)/test
BUILD := $(shell pwd)/build
VERSION := $(MAJOR).$(MINOR).$(GIT_TAG)
LIB_NAME := $(NAME).so.$(VERSION)
LIB := $(BUILD)/$(LIB_NAME)

CFLAGS := -O3 -g -Wall -Werror -I$(SRC)
CC := gcc

.PHONY: all test perf fake_orch

all: $(LIB) test

# Quickly test whether linked-list implementation is fast enough for
# our purposes.
perf: $(BUILD)/perf
	$^

$(BUILD):
	mkdir -p $(BUILD)

$(LIB): $(SRC)/*.[ch] | $(BUILD)
	$(CC) $(CFLAGS) -fPIC -shared -Wl,-z,defs -Wl,--as-needed -Wl,-soname,$(LIB_NAME) $(filter %.c, $^) -o $@ -lc

clean:
	$(RM) -rf $(BUILD)

TESTPROGS := $(addprefix $(BUILD)/,linked_list_test curl_test)

test: $(LIB)
	$(MAKE) CFLAGS="-DDEBUG -g -I$(SRC)" $(TESTPROGS)
	$(BUILD)/linked_list_test
	LD_AUDIT=$(LIB) $(BUILD)/curl_test

$(BUILD)/linked_list_test: $(TEST)/linked_list_test.c $(SRC)/linked_list.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD)/curl_test: $(TEST)/curl_test.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@ -lcurl

$(BUILD)/perf: $(TEST)/perf.c $(SRC)/linked_list.c | $(BUILD)
	$(CC) $(CFLAGS) $^ -o $@

fake_orch:
	netcat -kul 51712

# Used during distribution building
dist: clean $(LIB)
	echo $(LIB_NAME) >/tmp/libcurl-agent.txt
