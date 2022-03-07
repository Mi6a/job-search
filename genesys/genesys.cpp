// genesys.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <strstream>
#include <string>
#include <array>

using namespace std::string_literals;

//
class Interpreter {
public:
   Interpreter() { 
      reset(); 
   }

   void reset(); // reset the interpreter

   void setNextMemory(int num) {
      _mem[_write++] = num;
   }

   // run content in memory
   int run();

private:
   void execute(int instr);

   std::array<int, 10> _regs; // registers
   std::array<int, 1000> _mem; // registers
   int _write; // position where next setNextMemory() will happen 
   int _exec; // position of next executing instruction
   bool _halt; // executed halt command
};

//
int main() {
   // read standard input and initialize interpreter
   Interpreter mach;
   
   // read number of cases
   int cntCases = 0;
   std::cin >> cntCases;
   for (int i = 0; i < cntCases; i++) {
      mach.reset();
      // read each case
      std::string line;
      std::getline(std::cin, line);
      // there is a confusion in text descrption and example input
      // lets tolerate for that
      if((0 == i) && line.empty())
         std::getline(std::cin, line);
      while (!line.empty()) {
         int instr = stoi(line);
         mach.setNextMemory(instr);
         std::getline(std::cin, line);
      }
      int res = mach.run(); // for now
      std::cout << res << std::endl;
   }
   return 0;
}

//
void Interpreter::reset() { // reset the interpreter
   for (int& cur : _regs)
      cur = 0;
   for (int& cur : _mem)
      cur = 0;
   _write = 0;
   _exec = 0;
   _halt = false;
}

//
int Interpreter::run() { // reset the interpreter
   int qntInstr = 0;
   while (!_halt && (_exec < _write)) { // let be safe still
      int instr = _mem[_exec++];
      try {
         execute(instr);
      }
      catch (std::string err) {
         std::cout << "error: '" << err << "' executing: " << instr << " at address: " << _exec - 1 << std::endl;
         _halt = true;
      }
      qntInstr++;
   }
   return qntInstr;
}

//
void Interpreter::execute(int instr) {
   // parse instruction
   int p2 = instr % 10;
   instr /= 10;
   int p1 = instr % 10;
   int icod = instr / 10;

   // for now we just go straight with switch. 
   // however much cleaner and probably faster would be with array of member function
   int tmp = 0;
   switch (icod) {
   case 1: // 100 means halt
      // both params should be 0 // or we shell ignore their values
      if (p2 || p1)
         throw "invalid instruction"s; // ??? 
      _halt = true;
      break;

   case 2: // 2dn means set register d to n (between 0 and 9)
      _regs[p1] = p2;
      break;

   case 3: // 3dn means add n to register d
      tmp = _regs[p1] + p2;
      _regs[p1] = tmp % 1000;
      break;

   case 4: // 4dn means multiply register d by n
      tmp = _regs[p1] * p2;
      _regs[p1] = tmp % 1000;
      break;

   case 5: // 5ds means set register d to the value of register s
      _regs[p1] = _regs[p2];
      break;

   case 6: // 6ds means add the value of register s to register d
      tmp = _regs[p1] + _regs[p2];
      _regs[p1] = tmp % 1000;
      break;

   case 7: // 7ds means multiply register d by the value of register s
      tmp = _regs[p1] * _regs[p2];
      _regs[p1] = tmp % 1000;
      break;

   case 8: // 8da means set register d to the value in RAM whose address is in register a
      _regs[p1] = _mem[_regs[p2]];
      break;

   case 9: // 9sa means set the value in RAM whose address is in register a to the value of register s
      _mem[_regs[p2]] = _regs[p1];
      break;

   case 0: // 0ds means goto the location in register d unless register s contains 0
      if (_regs[p2])
         _exec = _regs[p1];
      break;

   }
}
