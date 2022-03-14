// StackPath assignment
#include <iostream>
#include <atomic>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

#define NDEBUG
#include <cassert>

using std::cout; using std::endl;
using std::atomic;
using std::vector;
using std::thread;
using std::mutex;

using namespace std::chrono_literals;

//
class Pump {
public:
   Pump() { clear(); }

   bool occupy(int idCar);
   void release(int idCar);
   void clear() { _occupied = false; _countFillUps = 0; _idCar = -1; }

   unsigned countFillUps() { return _countFillUps; }

   int car() const { return _idCar; }

private:
   bool _occupied; // false/true/ - vacant/occupied
   unsigned _countFillUps;
   int _idCar; // for debugging purposes
};

class GasStation; 

//
class Car {
public:
   Car() : _id(0), _pump(-1), _countFillUps(0), _gs(0), _wakeup(false) {}
   Car(int id, GasStation* gs) : _id(id), _pump(-1), _countFillUps(0), _gs(gs), _wakeup(false) {}
   
   void set(int id, GasStation* gs);

   static void start(Car* c);
   void run();

   void wakeup();

   unsigned countFillUps() { return _countFillUps; }

   int id() const { return _id; }

private:
   void fillUp();

   int _id;
   int      _pump; // if -1 - waiting in queue, >= 0 - filling up on that pump
   unsigned _countFillUps;
   GasStation* _gs;
   bool _wakeup;

   mutex _mtx;
   std::condition_variable _cv;
};

enum EventType {}; // for logging

//
class GasStation {
public:
   GasStation(int numCars);

   void start();
   void stop();
   bool stopped() const { return _timeout; }

   int occupyPump(int idCar); // try to occupy one
   void releasePump(int idPump, int idCar);

   void notifyMovedToPump(int idCar, int numPipe); // head car notifies that he moved to pump 
   void notifyReleasePump(int idCar, int numPipe); // car notifies that it released the pump

   void printResults();

   void carFinished(int idCar) {
      _carsFinished++;
   }

private:
   void verifyCarToPump(int idCar, int numPipe);

   static const int PumpCount = 2;
   Pump _pumps[PumpCount];

   using CarList = vector<Car>;

   CarList _cars;
   int  _head; // current index of the head car
   int _numCars;
   atomic<int> _carsFinished; // number of thread cars finished
   bool _timeout; // when 30 sec done 

   mutex _mxCars; // protect head car setting
   mutex _mxPump; // use for pump' occupy/release
   std::condition_variable _cvPump; // use for pump' release events

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
      lock.unlock();
      if (_gs->stopped())
         break;
      _pump = _gs->occupyPump(_id); // locking 
      // advance to the pump
      _gs->notifyMovedToPump(_id, _pump);
      fillUp();
      _gs->notifyReleasePump(_id, _pump);
      _pump = -1;
   }
   _gs->carFinished(_id);
}

//
void Car::fillUp() {
   std::this_thread::sleep_for(30ms); // fill up
   _countFillUps++;
}

//
void Car::start(Car* c) {
   thread th(&Car::run, c);
   th.detach();
}

//
void Car::wakeup() {
   std::unique_lock<mutex> lock(_mtx);
   _wakeup = true;
   _cv.notify_one();
}

void Car::set(int id, GasStation* gs) {
   _id = id; _pump = -1; _countFillUps = 0; _gs = gs;
   _wakeup = false;
}

//
GasStation::GasStation(int numCars) : _cars(numCars), _head(0), _numCars(numCars), _carsFinished(0), _timeout(false) {
   for (int n = 0; n < numCars; n++) {
      _cars[n].set(n, this);
   }
}

// 
void GasStation::start() {
   // allow for multiple start()/stop() cicles  
   std::unique_lock<mutex> lkCars(_mxCars);
   _head = 0;
   _timeout = false;
   _carsFinished = 0;
   unsigned headCur = _head;
   lkCars.unlock();
   std::unique_lock<mutex> lkPump(_mxPump);
   _pumps[0].clear();
   _pumps[1].clear();
   lkPump.unlock();
   for(Car& cp: _cars)
      Car::start(&cp);
   _cars[headCur].wakeup();
}

//
void GasStation::stop() {
   std::unique_lock<mutex> lock(_mxCars); // use it as a barrier
   _timeout = true;
   lock.unlock();
   for (Car& car : _cars)
      car.wakeup();
   // wait for all car threads to finish
   while (_numCars > _carsFinished.load()) {
      std::this_thread::sleep_for(1ms);
   }
}

//
int GasStation::occupyPump(int idCar) {
   std::unique_lock<mutex> lock(_mxPump); 
   while (true) {
      for (int n = 0; n < PumpCount; n++) {
         if (_pumps[n].occupy(idCar))
            return n;
      }
      _cvPump.wait(lock);
   }
   return -1; 
}

//
void GasStation::releasePump(int idPump, int idCar) {
   assert((idPump >= 0) && (idPump < PumpCount));
   assert(_pumps[idPump].car() == idCar);
   _pumps[idPump].release(idCar);
}

//
void GasStation::verifyCarToPump(int idCar, int numPipe) {
   std::unique_lock<mutex> lock(_mxPump);
   assert(_pumps[numPipe].car() == idCar);
}

//
void GasStation::notifyMovedToPump(int idCar, int numPump) {
   verifyCarToPump(idCar, numPump);
   std::unique_lock<mutex> lock(_mxCars);
   assert(_head == idCar);
   _head++;
   if (_head == _numCars)
      _head = 0;
   unsigned headCur = _head;
   lock.unlock(); // avoid taking 2 mutexes at a time
   _cars[headCur].wakeup();
}

//
void GasStation::notifyReleasePump(int idCar, int numPipe) {
   std::unique_lock<mutex> lock(_mxPump);
   releasePump(numPipe, idCar);
   _cvPump.notify_one(); // at every moment only one should wait
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
bool Pump::occupy(int idCar) {
   if (_occupied)
      return false;
   _occupied = true;
   _countFillUps++;
   _idCar = idCar;
   return true; 
}

//
void Pump::release(int idCar) {
   assert(_idCar == idCar);
   _occupied = false;
   _idCar = -1;
}