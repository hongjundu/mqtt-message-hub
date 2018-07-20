.PHONY: all clean pre-build


PRJ_DIR = $(CURDIR)
export PRJ_DIR

ROOT_DIR = $(CURDIR)/..
export ROOT_DIR

DIR_LIBS = $(ROOT_DIR)/build/$(TARGET_FOLDER)/lib
export DIR_LIBS

export COMPILER=gcc
subdirs = zlog mqtt-c utils httpd json-c libtommath-1.0 libtomcrypt-1.17

include $(ROOT_DIR)/make/make.subdirs
