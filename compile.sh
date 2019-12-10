cp MachineHarness.cpp msgOS
cd msgOS
g++ -o mOS MachineHarness.cpp utils.cpp config.cpp -g
rm MachineHarness.cpp
mv mOS ..
