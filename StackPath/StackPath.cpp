#include <iostream>
#include <atomic>
#include <memory>
#include <list>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

using std::cout; using std::endl;
using std::atomic;
using std::shared_ptr;
using std::list;
using std::thread;
using std::mutex;

using namespace std::chrono_literals;

//
class Pump {
public:
   Pump() : _occupied(false), _countFillUps(0) {}

   bool occupy();
   void release();

   unsigned countFillUps() { return _countFillUps; }

private:
   atomic<bool> _occupied; // false/true/ - vacant/occupied
   unsigned _countFillUps;
};

class GasStation; 

//
class Car {
public:
   Car(unsigned id, GasStation* gs) : _id(id), _pump(-1), _countFillUps(0), _gs(gs), _wakeup(false) {}

   static void start(Car* c);
   void run();

   void wakeup();

   unsigned countFillUps() { return _countFillUps; }

   unsigned id() const { return _id; }

private:
   const unsigned _id;
   int      _pump; // if -1 - waiting in queue, >= 0 - filling up on that pump
   unsigned _countFillUps;
   GasStation* _gs;
   bool _wakeup;

   mutex _mtx;
   std::condition_variable _cv;
};

//
class GasStation {
public:
   GasStation(unsigned numCars);

   void start();
   void stop();
   bool stopped() const { return _timeout; }

   void wakeupHeadCar();

   int occupy(); // try to occupy one
   void release(int);

   void notifyMovedToPump(); // head car notifies that he moved to pump 
   void notifyReleasePump(); // car notifies that it released the pump

   void printResults();

   void carFinished() { 
      _carsFinished++;
   }

private:
   static const unsigned PumpCount = 2;
   Pump _pumps[PumpCount];

   using CarList = list<Car>;
   CarList _cars;
   CarList::iterator  _head; // current head of the line
   unsigned _numCars;
   atomic<unsigned> _carsFinished; // number of thread cars finished
   bool _timeout; // when 30 sec done 

   mutex _mtx;
};

//
int main()
{
   GasStation gs{10};
   gs.start();
   std::this_thread::sleep_for(30s);
   gs.stop();
   gs.printResults();
}

//
void Car::run() {
   while (!_gs->stopped()) {
      std::unique_lock<mutex> lock(_mtx);
      _cv.wait(lock, [this] { return _wakeup || _gs->stopped(); });
      _wakeup = false;
      if (_gs->stopped())
         break;
      _pump = _gs->occupy();
      if (-1 == _pump)
         continue;
      // advance to the pump
      _gs->notifyMovedToPump();
      std::this_thread::sleep_for(30ms); // fill up
      _countFillUps++;
      _gs->release(_pump);
      _pump = -1;
      _gs->notifyReleasePump();
   }
   _gs->carFinished();
}

//
void Car::start(Car* c) {
   thread th(&Car::run, c);
   th.detach();
}

//
void Car::wakeup() {
   _wakeup = true;
   _cv.notify_one();
}

//
GasStation::GasStation(unsigned numCars) : _numCars(numCars), _carsFinished(0), _timeout(false) {
   for (unsigned n = 0; n < numCars; n++) {
      _cars.emplace_back(n, this);
   }
   _head = _cars.begin();
}

// 
void GasStation::start() {
   // allow for multiple start()/stop() cicles  
   _head = _cars.begin();
   _timeout = false;
   _carsFinished = 0;
   for(Car& cp: _cars)
      Car::start(&cp);
   wakeupHeadCar();
}

//
void GasStation::stop() {
   _timeout = true;
   for (Car& car : _cars)
      car.wakeup();
   // wait for all car threads to finish
   while (_numCars > _carsFinished.load()) {
      std::this_thread::sleep_for(1ms);
   }
}

//
int GasStation::occupy() {
   for (unsigned n = 0; n < PumpCount; n++) {
      if (_pumps[n].occupy())
         return n;
   }
   return -1; 
}

//
void GasStation::release(int n) {
   _pumps[n].release();
}

//
void GasStation::wakeupHeadCar() {
   std::unique_lock<mutex> lock(_mtx);
   _head->wakeup();
}

//
void GasStation::notifyMovedToPump() {
   std::unique_lock<mutex> lock(_mtx);
   _head++;
   if (_cars.end() == _head)
      _head = _cars.begin();
   _head->wakeup();
}

//
void GasStation::notifyReleasePump() {
   wakeupHeadCar();
}

//
void GasStation::printResults() {
   for (Car& car: _cars) {
      cout << "Car " << car.id() << " : " << car.countFillUps() << endl;
   }
   for (unsigned n = 0; n < PumpCount; n++) {
      cout << "Pipe: " << n << " : " << _pumps[n].countFillUps() << endl;
   }
}

//
bool Pump::occupy() {
   bool occupiedNot{false};
   bool occupy{true};

   bool res = _occupied.compare_exchange_strong(occupiedNot, occupy);
   if (res)
      _countFillUps++;
   return res; 
}

//
void Pump::release() {
   _occupied = false;
}