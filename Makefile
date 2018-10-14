
librevoke.so : test.o
	g++ -shared -o librevoke.so test.o
test.o : test.cpp revoke.h
	g++ -std=c++17 -fPIC -c test.cpp -o test.o

test.exe : test.cs revoke.cs librevoke.so
	csc test.cs revoke.cs

run : test.exe
	LD_LIBRARY_PATH=. mono test.exe

clean :
	rm test.o test.exe librevoke.so

