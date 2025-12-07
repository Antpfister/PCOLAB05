#include "bikestation.h"
#include "bikinginterface.h"

BikeStation::BikeStation(int _capacity) : capacity(_capacity), 
      bikesByType(Bike::nbBikeTypes) {}
extern BikingInterface* binkingInterface;

BikeStation::~BikeStation() {
    ending();
}

void BikeStation::putBike(Bike* _bike){
    // index par rapport au type du velo pour bikesbytype et condtakers
    size_t t = _bike->bikeType;

    mutex.lock();

    // boucle condition mesa
    // NOTE: we calculate total bikes here instead of calling nbBikes() to avoid
    // deadlock since we already hold the mutex and nbBikes() would try to lock
    // it again
    while (!shouldEnd) {
        size_t total = 0;
        for (const auto& bikes : bikesByType) {
            total += bikes.size();
        }
        if (total < capacity) break;
        
        // condputter libère le mutex et attends un notifyOne
        condPutters.wait(&mutex);
    }
    // variable pour annuler l'action en cours
    if (shouldEnd) {
        mutex.unlock();
        return;
    }

    // place velo par type
    bikesByType[t].push_back(_bike);

    // reveille pour prendre un velo avec le même type
    condTakers[t].notifyOne();
    // reveille pour mette un velo
    condPutters.notifyOne();

    mutex.unlock();
}

Bike* BikeStation::getBike(size_t _bikeType)
{
    mutex.lock();

    // Attente tant qu'il n'y a pas de vélo du type demandé ET que la simulation n'est pas terminée
    while (!shouldEnd && bikesByType[_bikeType].empty()) {
        condTakers[_bikeType].wait(&mutex);
    }

    // Si la simulation est terminée → on sort proprement
    if (shouldEnd) {
        mutex.unlock();
        return nullptr;
    }

    // On prend le vélo (le premier arrivé = FIFO grâce à std::deque)
    Bike* bike = bikesByType[_bikeType].front();
    bikesByType[_bikeType].pop_front();

    // === Réveils obligatoires (Mesa style) ===
    // 1. On a libéré une place → quelqu’un peut rendre un vélo
    condPutters.notifyOne();

    // 2. On réveille aussi un éventuel autre preneur du même type
    //     (même si c'est vide maintenant → il se rendormira grâce au while → correct en Mesa)
    condTakers[_bikeType].notifyOne();

    mutex.unlock();
    return bike;
}

std::vector<Bike*> BikeStation::addBikes(std::vector<Bike*> _bikesToAdd) {
    std::vector<Bike*> result;
    
    mutex.lock();
    
    // try to add each bike from the vector
    for (Bike* bike : _bikesToAdd) {
        if (shouldEnd) {
            result.push_back(bike);
            continue;
        }
        
        // wait if station is full
        // NOTE: we calculate total bikes here instead of calling nbBikes() to
        // avoid deadlock since we already hold the mutex
        while (!shouldEnd) {
            size_t total = 0;
            for (const auto& bikes : bikesByType) {
                total += bikes.size();
            }
            if (total < capacity) break;
            
            // wait for space to become available
            condPutters.wait(&mutex);
        }
        
        // if simulation ended while waiting, add bike to result
        if (shouldEnd) {
            result.push_back(bike);
            continue;
        }
        
        size_t t = bike->bikeType;
        bikesByType[t].push_back(bike);
        
        // wake waiting threads
        condTakers[t].notifyOne();  // wake someone waiting for this type
        condPutters.notifyOne();    // wake someone waiting to put a bike
    }
    
    mutex.unlock();
    return result;
}

std::vector<Bike*> BikeStation::getBikes(size_t _nbBikes) {
    std::vector<Bike*> result;
    
    mutex.lock();
    
    // track which types we took from (for waking appropriate threads)
    bool tookFromType[Bike::nbBikeTypes] = { false };
    
    // take bikes in order of type (0, 1, 2...) and FIFO within each type
    for (size_t type = 0; type < Bike::nbBikeTypes && result.size() < _nbBikes; ++type) {
        while (!bikesByType[type].empty() && result.size() < _nbBikes) {
            Bike* bike = bikesByType[type].front();
            bikesByType[type].pop_front();
            result.push_back(bike);
            tookFromType[type] = true;
        }
    }
    
    // wake waiting threads if we took any bikes
    if (!result.empty()) {
        // wake putters (we freed up space)
        condPutters.notifyAll();
        
        // wake takers for each type we took from
        for (size_t type = 0; type < Bike::nbBikeTypes; ++type) {
            if (tookFromType[type]) {
                condTakers[type].notifyOne();
            }
        }
    }
    
    mutex.unlock();
    return result;
}

size_t BikeStation::countBikesOfType(size_t type) const {
    mutex.lock();
    size_t count = bikesByType[type].size();
    mutex.unlock();
    return count;
}

size_t BikeStation::nbBikes() {
    mutex.lock();
    size_t total = 0;
    for (const auto& bikes : bikesByType) {
        total += bikes.size();
    }
    mutex.unlock();
    return total;
}

size_t BikeStation::nbSlots() {
    return capacity;
}

void BikeStation::ending() {
    mutex.lock();
    shouldEnd = true;
    
    // wake all waiting threads (putters and takers)
    condPutters.notifyAll();
    for (size_t i = 0; i < Bike::nbBikeTypes; ++i) {
        condTakers[i].notifyAll();
    }
    
    mutex.unlock();
}
