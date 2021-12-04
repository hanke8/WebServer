CXX = g++
CFLAGS = -I../code/include/ -std=c++14
# CFLAGS = -I../code/include/ -std=c++14 -O2 -Wall -g 

TARGET = server
OBJS = ../code/server/*.cc ../code/http/*.cc  \
       ../code/buffer/*.cc ../code/main.cc

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)