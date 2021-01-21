all:
	mkdir -p build && cd build && cmake .. && make

clean:
	rm -rf build


ipsec:
	cd intel-ipsec-mb && make SHARED=n

ipsec-debug:
	cd intel-ipsec-mb && make DEBUG=y SHARED=n

clean-ipsec:
	cd intel-ipsec-mb && make clean SHARED=n
