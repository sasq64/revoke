OS := $(shell uname)

cmakeproj :
	make -C build

test.exe : test.cs revoke.cs cmakeproj
	csc test.cs revoke.cs

run : test.exe
	LD_LIBRARY_PATH=build mono test.exe

clean :
	rm test.exe
	make -C build clean
