# makefile for embedded-console

export TOP_DIR := $(shell pwd)

include $(TOP_DIR)/config.mk


.PHONY: all test test-build clean

all:
	make -C src

test: test-build
	./tests/shell-wrap ./tests/econ_test $(TEST_ARGS)

test-build: all
	make -C tests

clean:
	make -C src clean
	make -C tests clean
