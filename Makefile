OS := $(shell uname)
ifeq ($(OS),Darwin)
	LINK_FLAGS :=
else
	LINK_FLAGS := -Wl,-Bstatic -static-libgcc -static-libstdc++
endif

librevoke.so : test.o
	g++ -shared ${LINK_FLAGS} -llua -o librevoke.so test.o
test.o : test.cpp revoke.h resol.h
	g++ -std=c++14 -fPIC -c -Ilua test.cpp -o test.o

test.exe : test.cs revoke.cs librevoke.so
	csc test.cs revoke.cs

run : test.exe
	LD_LIBRARY_PATH=. mono test.exe

clean :
	rm test.o test.exe librevoke.so

