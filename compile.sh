mkdir -p bin

cd corpus/programs
nodejs ../../compilers/JavaScript/compiler.js init.chi
mv init.kua ../../bin
cd ../..

cp Chika_PC.cpp Chika_Arduino
cd Chika_Arduino
g++ -o ../bin/chika Chika_PC.cpp ChVM.cpp Item.cpp utils.cpp config.cpp -g
rm Chika_PC.cpp

echo "Compiled Chika VM for PC."
