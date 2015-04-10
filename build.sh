
echo =================================
echo  Todo.txt Parser -- Build Script
echo =================================

cd build
cd linux64

# TODO: Proper build script
g++ ../../code/linux_todo.cpp -o todo -ggdb

echo Build Complete