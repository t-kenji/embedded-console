# makefile for embedded-console source

include $(TOP_DIR)/config.mk

TARGET := lib$(NAME).a

ifeq ($(PLATFORM),quatro55xx)
  CROSS_COMPILE := arm-linux-gnueabihf-
  OPT_ARCH :=
else
  OPT_ARCH := -m32
endif

INCS := -I. -I$(TOP_DIR)/include
OPT_WARN := -Wall -Werror
OPT_OPTIM := -Og
OPT_DEBUG := -g
OPT_DEP := -MMD -MP
OPTS := $(OPT_WARN) $(OPT_OPTIM) $(OPT_DEBUG) $(OPT_DEP)

CPPFLAGS := -DNODEBUG=$(NODEBUG)
CPPFLAGS += $(EXTRA_CPPFLAGS)
CFLAGS := -std=c11 $(OPTS) $(INCS)
CFLAGS += $(EXTRA_CFLAGS)

CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar rcs

SRCS := econ.c
DEPS := $(SRCS:.c=.d)
OBJS := $(SRCS:.c=.o)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

.PHONY: all $(TARGET) clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) $@ $^

clean:
	rm -rf $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
