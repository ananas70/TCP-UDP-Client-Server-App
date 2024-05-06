CC := g++
CFLAGS := -Wall -Wextra -std=c++17

SRCDIR := src
BUILDDIR := build
TARGETS := server subscriber

COMMON_SRCS := $(SRCDIR)/common.c
COMMON_OBJS := $(BUILDDIR)/common.o

SERVER_SRCS := $(SRCDIR)/server.cpp
SERVER_OBJS := $(BUILDDIR)/server.o

SUBSCRIBER_SRCS := $(SRCDIR)/subscriber.cpp
SUBSCRIBER_OBJS := $(BUILDDIR)/subscriber.o

TYPES_SRCS := $(SRCDIR)/types.h

.PHONY: all clean

all: $(TARGETS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

server: $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

subscriber: $(SUBSCRIBER_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILDDIR) $(TARGETS)
