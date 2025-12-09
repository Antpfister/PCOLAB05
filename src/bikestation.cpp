/*
* Auteur : Anthony Pfister, Santiago Sugranes
 * Date : 09.12.2025
 * PCO2025 lab05
 * HEIG
 */

#include "bikestation.h"
#include "bikinginterface.h"

BikeStation::BikeStation(int _capacity) : capacity(_capacity), 
      bikesByType(Bike::nbBikeTypes) {}
extern BikingInterface* binkingInterface;

BikeStation::~BikeStation() {
    ending();
}
// Put a bike into the station
void BikeStation::putBike(Bike* _bike){
    size_t t = _bike->bikeType; // get bike type (used for indexing bikesByType and condTakers)

    mutex.lock(); // lock the mutex to protect shared data

    // Mesa-style waiting: loop until there is space or simulation ends
    while (!shouldEnd) {
        size_t total = 0;
        for (const auto& bikes : bikesByType) {
            total += bikes.size(); // count total bikes in station
        }
        if (total < capacity) break; // if space available, exit the loop

        condPutters.wait(&mutex); // release mutex and wait until someone signals (blocking wait)
    }

    if (shouldEnd) { // if simulation ended while waiting
        mutex.unlock(); // unlock and exit
        return;
    }

    bikesByType[t].push_back(_bike); // put bike in the deque of its type

    // wake one thread waiting to take a bike of this type
    condTakers[t].notifyOne();
    // wake one thread waiting to put a bike (space freed)
    condPutters.notifyOne();

    mutex.unlock(); // unlock after modifying shared data
}

// Get a bike of a specific type
Bike* BikeStation::getBike(size_t _bikeType)
{
    mutex.lock(); // lock mutex to access shared data

    // wait until a bike of the requested type is available or simulation ends
    while (!shouldEnd && bikesByType[_bikeType].empty()) {
        condTakers[_bikeType].wait(&mutex); // blocking wait
    }

    if (shouldEnd) { // if simulation ended while waiting
        mutex.unlock();
        return nullptr; // return null pointer to indicate no bike
    }

    Bike* bike = bikesByType[_bikeType].front(); // get the first bike (FIFO)
    bikesByType[_bikeType].pop_front();          // remove it from the deque

    // wake one thread waiting to put a bike (space freed)
    condPutters.notifyOne();

    // wake another thread waiting for this type (Mesa style)
    condTakers[_bikeType].notifyOne();

    mutex.unlock(); // unlock mutex
    return bike;    // return the bike
}

// Add multiple bikes at once
std::vector<Bike*> BikeStation::addBikes(std::vector<Bike*> _bikesToAdd) {
    std::vector<Bike*> result; // bikes that couldn't be added (if simulation ends)

    mutex.lock(); // lock shared data

    for (Bike* bike : _bikesToAdd) { // try each bike
        if (shouldEnd) { // simulation ended
            result.push_back(bike); // keep it in result
            continue;
        }

        // wait while station is full
        while (!shouldEnd) {
            size_t total = 0;
            for (const auto& bikes : bikesByType) {
                total += bikes.size();
            }
            if (total < capacity) break;

            condPutters.wait(&mutex); // blocking wait for space
        }

        if (shouldEnd) { // simulation ended while waiting
            result.push_back(bike);
            continue;
        }

        size_t t = bike->bikeType;
        bikesByType[t].push_back(bike); // add bike

        // wake threads waiting for this type or for space
        condTakers[t].notifyOne();
        condPutters.notifyOne();
    }

    mutex.unlock(); // unlock
    return result;  // return bikes that couldn't be added
}

// Get multiple bikes at once
std::vector<Bike*> BikeStation::getBikes(size_t _nbBikes) {
    std::vector<Bike*> result;

    mutex.lock(); // lock shared data

    bool tookFromType[Bike::nbBikeTypes] = { false }; // track types we took bikes from

    // iterate over bike types in order
    for (size_t type = 0; type < Bike::nbBikeTypes && result.size() < _nbBikes; ++type) {
        while (!bikesByType[type].empty() && result.size() < _nbBikes) {
            Bike* bike = bikesByType[type].front(); // take first bike
            bikesByType[type].pop_front();
            result.push_back(bike);
            tookFromType[type] = true;
        }
    }

    if (!result.empty()) {
        condPutters.notifyAll(); // wake all threads waiting to put a bike
        for (size_t type = 0; type < Bike::nbBikeTypes; ++type) {
            if (tookFromType[type]) {
                condTakers[type].notifyOne(); // wake one waiting for this type
            }
        }
    }

    mutex.unlock();
    return result; // return bikes
}

// Count bikes of a specific type
size_t BikeStation::countBikesOfType(size_t type) const {
    mutex.lock();
    size_t count = bikesByType[type].size(); // get size
    mutex.unlock();
    return count;
}

// Count total bikes
size_t BikeStation::nbBikes() {
    mutex.lock();
    size_t total = 0;
    for (const auto& bikes : bikesByType) {
        total += bikes.size();
    }
    mutex.unlock();
    return total;
}

// Return station capacity
size_t BikeStation::nbSlots() {
    return capacity;
}

// Signal all threads that simulation is ending
void BikeStation::ending() {
    mutex.lock();
    shouldEnd = true; // mark end

    condPutters.notifyAll(); // wake all putters
    for (size_t i = 0; i < Bike::nbBikeTypes; ++i) {
        condTakers[i].notifyAll(); // wake all takers
    }

    mutex.unlock();
}