#!/bin/sh
mkdir -p bin

#Compile the Chika executable
cd Chika_Arduino
g++ -o ../bin/chika Chika_PC.cpp ChVM.cpp Broker.cpp Item.cpp Compiler.cpp utils.cpp config.cpp -g -Wall -Wno-switch #-O3

cd ../bin
#Copy the init program if it doesn't yet exist
if [ ! -f init.chi ]; then
  cp ../corpus/programs/init.chi .
fi
#Compile all .chi in the bin folder
for filename in *.chi; do
  [ -e "$filename" ] || continue
   ./chika c $filename
done
cd ..

echo "Compiled Chika VM for PC, and *.chi in bin/."