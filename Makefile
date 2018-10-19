OS := $(shell uname)

all : test.exe

build :
	cmake -Bbuild -H.

compile_commands.json : build/compile_commands.json
	ln -s build/compile_commands.json .

cmakeproj : build compile_commands.json
	make -C build

test.exe : test.cs revoke.cs cmakeproj
	csc test.cs revoke.cs

run : test.exe
	LD_LIBRARY_PATH=build mono test.exe

clean :
	rm test.exe
	make -C build clean
