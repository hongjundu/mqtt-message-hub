.PHONY: all clean

PRJ_DIR = $(CURDIR)
export PRJ_DIR

ROOT_DIR = $(CURDIR)/..
export ROOT_DIR

SHARE_LIBS_DIR = $(ROOT_DIR)/sharelibs
export SHARE_LIBS_DIR

UTILS_DIR = $(SHARE_LIBS_DIR)/utils
export UTILS_DIR

ZLOG_DIR = $(SHARE_LIBS_DIR)/zlog
export ZLOG_DIR

JSONC_DIR = $(SHARE_LIBS_DIR)/json-c
export JSONC_DIR

MQTTC_DIR = $(SHARE_LIBS_DIR)/mqtt-c
export MQTTC_DIR

HTTPD_DIR = $(SHARE_LIBS_DIR)/httpd
export HTTPD_DIR

SHARE_DIR = $(SHARE_LIBS_DIR)/share
export SHARE_DIR

MQTTC_DIR = $(SHARE_LIBS_DIR)/mqtt-c
export MQTTC_DIR

TOMCRYPT_DIR = $(SHARE_LIBS_DIR)/libtomcrypt-1.17
export TOMCRYPT_DIR

APP_DIR = $(PRJ_DIR)/hub-app
export APP_DIR

CIPHER_DIR = $(PRJ_DIR)/cipher
export CIPHER_DIR

subdirs = cipher hub-app main

INSTALL_FILES = $(PRJ_DIR)/conf/mqttmsghub.ini $(PRJ_DIR)/conf/zlog.conf
export INSTALL_FILES

ADD_CFLAGS = -DPAYLOAD_ENCRYPT
export ADD_CFLAGS

include $(ROOT_DIR)/make/make.subdirs

