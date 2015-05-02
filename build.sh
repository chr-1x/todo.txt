
echo =================================
echo  Todo.txt Parser -- Build Script
echo =================================

cd build
cd linux64

# TODO: Proper build script
g++ ../../code/linux_todo.cpp -I../../../chr -o todo -ggdb

echo Build Complete