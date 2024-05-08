CC := g++
CFLAGS := -Wall -Wextra -std=c++17

SRCDIR := src
BUILDDIR := build
TARGETS := server subscriber

GENERAL_SRCS := $(SRCDIR)/general_utils.c
GENERAL_OBJS := $(BUILDDIR)/general_utils.o

SUBSCRIPTION_SRCS := $(SRCDIR)/subscription.c
SUBSCRIPTION_OBJS := $(BUILDDIR)/subscription.o

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

server: $(SERVER_OBJS) $(GENERAL_OBJS) $(SUBSCRIPTION_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

subscriber: $(SUBSCRIBER_OBJS) $(GENERAL_OBJS) $(SUBSCRIPTION_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILDDIR) $(TARGETS)