#include "bikestation.h"
#include "bikinginterface.h"
#include <QMutex>
#include <QWaitCondition>

BikeStation::BikeStation(int _capacity) : capacity(_capacity) {}
extern BikingInterface* binkingInterface;

BikeStation::~BikeStation() {
    ending();
}

void BikeStation::putBike(Bike* _bike){
    // TODO: implement this method
    // index par rapport au type du velo pour bikesbytype et condtakers
    size_t t = _bike->bikeType;

    // mutex
    QMutexLocker locker(&mutex);                                // ← remplace unique_lock

    // boucle condition mesa
    while (!shouldEnd && nbBikes() >= capacity) {
        // condputter libère le mutex et attends un waitone
        condPutters.wait(&mutex);                           // ← wait Qt
    }
    // variable pour annuler l'action en cours
    if (shouldEnd) return;

    // place velo par type
    bikesByType[t].push_back(_bike);

    // reveille pour prendre un velo avec le même type
    condTakers[t].wakeOne();
    // reveille pour mette un velo
    condPutters.wakeOne();

}

Bike* BikeStation::getBike(size_t _bikeType)
{
    QMutexLocker locker(&mutex);

    // Attente tant qu'il n'y a pas de vélo du type demandé ET que la simulation n'est pas terminée
    while (!shouldEnd && bikesByType[_bikeType].empty()) {
        condTakers[_bikeType].wait(&mutex);   // ← Mesa + Qt
    }

    // Si la simulation est terminée → on sort proprement
    if (shouldEnd) return nullptr;

    // On prend le vélo (le premier arrivé = FIFO grâce à std::deque)
    Bike* bike = bikesByType[_bikeType].front();
    bikesByType[_bikeType].pop_front();

    // === Réveils obligatoires (Mesa style) ===
    // 1. On a libéré une place → quelqu’un peut rendre un vélo
    condPutters.wakeOne();

    // 2. On réveille aussi un éventuel autre preneur du même type
    //     (même si c'est vide maintenant → il se rendormira grâce au while → correct en Mesa)
    condTakers[_bikeType].wakeOne();

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
    // TODO: implement this method
    return 0;
}

size_t BikeStation::nbBikes() {
    // TODO: implement this method
    return 0;
}

size_t BikeStation::nbSlots() {
    return capacity;
}

void BikeStation::ending() {
   // TODO: implement this method
}
