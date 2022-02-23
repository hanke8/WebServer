all:
	mkdir -p bin
	cd build && make

.PHONY:clean
clean:
	rm -rf ./bin/server