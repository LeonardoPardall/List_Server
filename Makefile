PROTOC = $(PROTOC_DIR)/bin/protoc-c

SRC_DIR := source
INC_DIR := include
OBJ_DIR := object
LIB_DIR := lib
BIN_DIR := binary

CC := gcc
CFLAGS := -Wall -g -O2 -I$(INC_DIR)
LDFLAGS := -lprotobuf-c -lzookeeper_mt -lpthread
AR := ar
ARFLAGS := -rcs

SRCS_LIB := $(SRC_DIR)/data.c $(SRC_DIR)/list.c
OBJS_LIB := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS_LIB))

SRCS_CLIENT := $(SRC_DIR)/list_client.c $(SRC_DIR)/client_stub.c $(SRC_DIR)/network_client.c $(SRC_DIR)/sdmessage.pb-c.c $(SRC_DIR)/message-private.c $(SRC_DIR)/chain_client.c
OBJS_CLIENT := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS_CLIENT))

SRCS_SERVER := $(SRC_DIR)/list_server.c $(SRC_DIR)/network_server.c $(SRC_DIR)/list_skel.c $(SRC_DIR)/message-private.c $(SRC_DIR)/sdmessage.pb-c.c $(SRC_DIR)/list.c $(SRC_DIR)/data.c $(SRC_DIR)/client_stub.c $(SRC_DIR)/network_client.c $(SRC_DIR)/chain_server.c 
OBJS_SERVER := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS_SERVER))

PROTO_FILES := $(wildcard *.proto)
PB_C_FILES := $(PROTO_FILES:.proto=.pb-c.c)
PB_H_FILES := $(PROTO_FILES:.proto=.pb-c.h)
PB_OBJS := $(PB_C_FILES:.c=.o)

LIB := $(LIB_DIR)/liblist.a

.PHONY: all liblist clean

all: $(PB_C_FILES) $(PB_H_FILES) liblist list_client list_server

%.pb-c.c %.pb-c.h: %.proto | $(SRC_DIR) $(INC_DIR)
	$(PROTOC) --c_out=. $<
	mv $*.pb-c.h $(INC_DIR)/
	mv $*.pb-c.c $(SRC_DIR)/

list_client: $(OBJS_CLIENT) $(LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/list_client $(OBJS_CLIENT) $(LIB) $(LDFLAGS)

list_server: $(OBJS_SERVER) $(LIB) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(BIN_DIR)/list_server $(OBJS_SERVER) $(LIB) $(LDFLAGS)

liblist: $(LIB)

$(LIB): $(OBJS_LIB) | $(LIB_DIR)
	$(AR) $(ARFLAGS) $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR) $(LIB_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR)/*.o binary/* $(LIB_DIR)/*.a

