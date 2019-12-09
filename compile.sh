cp MachineHarness.cpp msgOS
cd msgOS
g++ -o msgOS.out MachineHarness.cpp utils.cpp config.cpp -g
rm MachineHarness.cpp
mv msgOS.out ..
