CC=g++
CFLAGS= -c -Wall

PROJECT_NAME = tcp_server
CURRENT_DIR=$(shell pwd)

INCLUDE_DIRS = -I$(CURRENT_DIR)
CXXFLAGS += $(INCLUDE_DIRS)

LDFLAGS=-lpthread
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)

all: $(SOURCES) $(PROJECT_NAME)

$(PROJECT_NAME): $(OBJECTS)
	$(CC) -g $(OBJECTS) $(LDFLAGS) -o $@
	rm *.o

.cpp.o:
	$(CC) -g $(CFLAGS) $(INCLUDE_DIRS) $< -o $@