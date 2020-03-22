#!/bin/sh
mkdir -p bin
cd bin
#Copy the init program if it doesn't yet exist
if [ ! -f init.chi ]; then
  cp ../corpus/programs/init.chi .
fi
#Compile all .chi in the bin folder
for filename in *.chi; do
  [ -e "$filename" ] || continue
  node ../compilers/JavaScript/compiler.js $filename
done
cd ..
#Compile the Chika executable
#  by temporarily copying Chika_PC.cpp into the Chika_Arduino directory
#  This is done as Arduino IDE wouldn't ignore the file during compilation
cp Chika_PC.cpp Chika_Arduino
cd Chika_Arduino
g++ -o ../bin/chika Chika_PC.cpp ChVM.cpp Broker.cpp Item.cpp utils.cpp config.cpp -g
rm Chika_PC.cpp

echo "Compiled Chika VM for PC."