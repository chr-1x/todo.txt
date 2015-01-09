#!/bin/sh


echo =================================
echo  Todo.txt Parser -- Build Script
echo =================================

cd build
cd x64

# TODO: Proper build script
gcc ../../code/linux_todo.cpp -o todo

echo Build Complete