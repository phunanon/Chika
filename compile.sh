rm -r bin
mkdir bin

cd Chika
nodejs compiler.js programs/init.chi
mv programs/init.kua ../bin
cd ..

cp MachineHarness.cpp msgOS
cd msgOS
g++ -o ../bin/mOS MachineHarness.cpp Item.cpp utils.cpp config.cpp -g
rm MachineHarness.cpp
