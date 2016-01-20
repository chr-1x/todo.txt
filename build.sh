#/bin/bash

echo =================================
echo  Todo.txt Parser -- Build Script
echo =================================

mkdir -p build
cd build
mkdir -p linux64
cd linux64

# TODO: Proper build script
g++ `pwd`/../../code/linux_todo.cpp -std=c++11 -o todo -ggdb

echo Build Complete
