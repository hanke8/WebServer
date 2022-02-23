CXX = g++
CFLAGS = -I../code/include/ -std=c++11 -pthread -O2
# CFLAGS = -I../code/include/ -std=c++14 -O2 -Wall -g 

TARGET = server
OBJS = ../code/server/*.cc ../code/http/*.cc ../code/timer/*.cc \
       ../code/buffer/*.cc ../code/main.cc

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)