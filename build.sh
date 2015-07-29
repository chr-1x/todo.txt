#/bin/bash

echo =================================
echo  Todo.txt Parser -- Build Script
echo =================================

cd build
cd linux64

# TODO: Proper build script
g++ `pwd`/../../code/linux_todo.cpp -I`pwd`/../../../../lib/chr -std=c++11 -o todo -ggdb

echo Build Complete
