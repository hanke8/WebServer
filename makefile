# all: svr clt
# src1=./test/testServer.cc
# src2=./server/webserver.cc
# src3=./server/epoller.cc

# svr: $(src1) $(src2) $(src3)
# 	g++ -o svr -I./server/ $(src1) $(src2) $(src3)

# clt: ./test/testClient.cc
# 	g++ -o clt ./test/testClient.cc

all:
	mkdir -p bin
	cd build && make

.PHONY:clean
clean:
	rm -rf ./bin/server