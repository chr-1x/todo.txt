all: linux64 linux32

linux64:
	@mkdir -p build
	@mkdir -p build/linux64
	g++ -std=c++11 code/linux_todo.cpp -o build/linux64/todo -ggdb

linux32:
	@mkdir -p build
	@mkdir -p build/linux32
	g++ -std=c++11 code/linux_todo.cpp -o build/linux32/todo -ggdb

clean:
	rm -rf build/linux64 build/linux32


