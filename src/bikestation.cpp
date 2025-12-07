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
    std::vector<Bike*> result; // Can be removed, it's just to avoid a compiler warning
    // TODO: implement this method
    return result;
}

std::vector<Bike*> BikeStation::getBikes(size_t _nbBikes) {
    std::vector<Bike*> result; // Can be removed, it's just to avoid a compiler warning
    // TODO: implement this method
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
