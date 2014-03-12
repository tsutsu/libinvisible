all: build

build: libinvisible.so

libinvisible.so: libinvisible.cc
	g++ -fpic -shared -std=c++11 -O3 -o libinvisible.so libinvisible.cc -lstdc++ -ldl

install: build
	cp libinvisible.so /usr/local/lib
	echo "/usr/local/lib/libinvisible.so" >> /etc/ld.so.preload

clean:
	rm -f libinvisible.so