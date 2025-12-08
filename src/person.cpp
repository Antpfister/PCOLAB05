#include "person.h"
#include "bike.h"
#include <random>

BikingInterface* Person::binkingInterface = nullptr;
std::array<BikeStation*, NB_SITES_TOTAL> Person::stations{};


Person::Person(unsigned int _id) : id(_id), homeSite(0), currentSite(0) {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, Bike::nbBikeTypes - 1);
    preferredType = dist(rng);

    if (binkingInterface) {
        log(QString("Person %1, préfère type %2")
                .arg(id).arg(preferredType));
    }
}

void Person::setStations(const std::array<BikeStation*, NB_SITES_TOTAL>& _stations){
    Person::stations = _stations;
}

void Person::setInterface(BikingInterface* _binkingInterface) {
    binkingInterface = _binkingInterface;
}


void Person::run() {
    // infinite loop: take bike -> ride -> deposit -> walk -> repeat
    while (true) {
        // 1. take a bike of preferred type from current site
        Bike* bike = takeBikeFromSite(currentSite);
        if (!bike) {
            // simulation is ending, exit cleanly
            log(QString("Person %1: simulation ending, exiting").arg(id));
            return;
        }
        
        // 2. ride to a different site j
        unsigned int siteJ = chooseOtherSite(currentSite);
        bikeTo(siteJ, bike);
        
        // 3. deposit the bike at site j
        depositBikeAtSite(siteJ, bike);
        
        // 4. walk to another site k (different from j)
        unsigned int siteK = chooseOtherSite(siteJ);
        walkTo(siteK);
        
        // 5. update current site (i <- k)
        // NOTE: walkTo() already updates currentSite, but we're explicit here
        // for clarity according to the instructions
    }
}

Bike* Person::takeBikeFromSite(unsigned int _site) {
    // get bike of preferred type from the station
    Bike* bike = stations[_site]->getBike(preferredType);
    
    if (!bike) {
        return nullptr;
    }
    
    // update GUI with new bike count
    if (binkingInterface) {
        binkingInterface->setBikes(_site, stations[_site]->nbBikes());
    }
    
    log(QString("Person %1: took bike type %2 from site %3")
        .arg(id).arg(preferredType).arg(_site));
    
    return bike;
}

void Person::depositBikeAtSite(unsigned int _site, Bike* _bike) {
    // deposit the bike at the station (waits if station is full)
    stations[_site]->putBike(_bike);
    
    // update GUI with new bike count
    if (binkingInterface) {
        binkingInterface->setBikes(_site, stations[_site]->nbBikes());
    }
    
    log(QString("Person %1: deposited bike at site %2")
        .arg(id).arg(_site));
}

void Person::bikeTo(unsigned int _dest, Bike* _bike) {
    unsigned int t = bikeTravelTime();
    if (binkingInterface) {
        binkingInterface->travel(id, currentSite, _dest, t);
    }
    currentSite = _dest;
}

void Person::walkTo(unsigned int _dest) {
    unsigned int t = walkTravelTime();
    if (binkingInterface) {
        binkingInterface->walk(id, currentSite, _dest, t);
    }
    currentSite = _dest;
}

unsigned int Person::chooseOtherSite(unsigned int _from) const {
    return randomSiteExcept(NBSITES, _from);
}

unsigned int Person::bikeTravelTime() const {
    return randomTravelTimeMs() + 1000;
}

unsigned int Person::walkTravelTime() const {
    return randomTravelTimeMs() + 2000;
}

void Person::log(const QString& msg) const {
    if (binkingInterface) {
        binkingInterface->consoleAppendText(id, msg);
    }
}

