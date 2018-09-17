# makefile for embedded-console tests

include $(TOP_DIR)/config.mk

TARGET := $(NAME)_test

ifeq ($(PLATFORM),quatro55xx)
  CROSS_COMPILE := arm-linux-gnueabihf-
  OPT_ARCH :=
else
  OPT_ARCH := -m32
endif

INCS := -I. -I$(TOP_DIR)/src -I$(TOP_DIR)/include
OPT_WARN := -Wall -Werror
OPT_OPTIM := -Og
OPT_DEBUG := -g
OPT_DEP := -MMD -MP
OPTS := $(OPT_WARN) $(OPT_OPTIM) $(OPT_DEBUG) $(OPT_DEP)

CPPFLAGS := -DNODEBUG=$(NODEBUG)
CPPFLAGS += $(EXTRA_CPPFLAGS)
CXXFLAGS := -std=c++11 $(OPTS) $(INCS)
CXXFLAGS += $(EXTRA_CXXFLAGS)
LDFLAGS := -L$(TOP_DIR)/src
LDFLAGS += $(EXTRA_LDFLAGS)
LIBS := -l$(NAME)
LIBS += $(EXTRA_LIBS)

CXX := $(CROSS_COMPILE)g++

SRCS := main.cpp
DEPS := $(SRCS:.cpp=.d)
OBJS := $(SRCS:.cpp=.o)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

.PHONY: all $(TARGET) clean

all: $(TARGET) shell-wrap

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

shell-wrap: shell-wrap.o
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf $(TARGET) $(DEPS) $(OBJS) shell-wrap shell-wrap.d shell-wrap.o

-include $(DEPS)
