#include <iostream>
#include <atomic>

using std::cout; using std::endl;
using std::atomic;

//
class Pump {
public:
   void occupy();
   void release();

private:
   atomic<bool> _busy; // make atomic
   unsigned _countFillUps;
};

//
class GasStation {
public:
   unsigned occupy();
   void release(unsigned);

private:
   static const unsigned PipeCount = 2;
   Pump _pumps[PipeCount];
};

// 
GasStation  gs; // for simplicity 

//
class Car {
public:
   Car() : _wait(true), _pos(0), _countFillUps(0) {}

   void initialize(unsigned pos);

   void start();
   void run();
   void join();

   static void stop() {
      _timeout = true;
   }

   unsigned countFillUps() { return _countFillUps; }

private:
   atomic<bool>     _wait; // if true - waiting in queue, false - filling up
   atomic<unsigned> _pos; // position in queue, 0-based. If filling up, not important 
   unsigned _countFillUps;

   static bool _timeout; // when 30 sec done 
};

bool Car::_timeout = false;

//
int main()
{
   static const unsigned NumCars = 10;
   Car cars[NumCars];
   for (unsigned n = 0; n < NumCars; n++) {
      cars[n].initialize(n);
      cars[n].start();
      // set timer for 30 seconds
      cars[n].join();
   }

}

