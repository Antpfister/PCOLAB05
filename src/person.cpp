/*
* Auteur : Anthony Pfister, Santiago Sugranes
 * Date : 09.12.2025
 * PCO2025 lab05
 * HEIG
 */

#include "person.h"
#include "bike.h"
#include <random>

// Static members initialization
BikingInterface* Person::binkingInterface = nullptr; // GUI/interface pointer
std::array<BikeStation*, NB_SITES_TOTAL> Person::stations{}; // all bike stations

// Constructor
Person::Person(unsigned int _id) : id(_id), homeSite(0), currentSite(0) {
    // Random generator for preferred bike type (thread-local to be safe in multi-threading)
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, Bike::nbBikeTypes - 1);
    preferredType = dist(rng); // assign a random preferred bike type

    // log initial preference if interface exists
    if (binkingInterface) {
        log(QString("Person %1, préfère type %2")
                .arg(id).arg(preferredType));
    }
}

// Set static stations array (all Persons share same stations)
void Person::setStations(const std::array<BikeStation*, NB_SITES_TOTAL>& _stations){
    Person::stations = _stations;
}

// Set the GUI/interface pointer (shared by all Persons)
void Person::setInterface(BikingInterface* _binkingInterface) {
    binkingInterface = _binkingInterface;
}

// Main loop of the Person (thread)
void Person::run() {
    // infinite loop: take bike -> ride -> deposit -> walk -> repeat
    while (true) {
        // 1. try to take a bike of preferred type from current site
        Bike* bike = takeBikeFromSite(currentSite);
        if (!bike) { // simulation ending
            log(QString("Person %1: simulation ending, exiting").arg(id));
            return; // exit thread
        }

        // 2. choose another site to go to
        unsigned int siteJ = chooseOtherSite(currentSite);
        bikeTo(siteJ, bike); // travel by bike

        // 3. deposit bike at destination
        depositBikeAtSite(siteJ, bike);

        // 4. choose another site to walk to
        unsigned int siteK = chooseOtherSite(siteJ);
        walkTo(siteK); // travel by walking

        // loop repeats indefinitely
    }
}

// Take a bike from a specific site
Bike* Person::takeBikeFromSite(unsigned int _site) {
    Bike* bike = stations[_site]->getBike(preferredType); // may block if no bike available

    if (!bike) { // station closed / simulation ending
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

// Deposit a bike at a specific site
void Person::depositBikeAtSite(unsigned int _site, Bike* _bike) {
    stations[_site]->putBike(_bike); // may block if station full

    // update GUI with new bike count
    if (binkingInterface) {
        binkingInterface->setBikes(_site, stations[_site]->nbBikes());
    }

    log(QString("Person %1: deposited bike at site %2")
        .arg(id).arg(_site));
}

// Travel by bike to a destination
void Person::bikeTo(unsigned int _dest, Bike* _bike) {
    unsigned int t = bikeTravelTime(); // compute travel time
    if (binkingInterface) {
        binkingInterface->travel(id, currentSite, _dest, t); // GUI animation
    }
    currentSite = _dest; // update current site
}

// Travel by walking to a destination
void Person::walkTo(unsigned int _dest) {
    unsigned int t = walkTravelTime(); // compute walking time
    if (binkingInterface) {
        binkingInterface->walk(id, currentSite, _dest, t); // GUI animation
    }
    currentSite = _dest; // update current site
}

// Choose a random site that is different from _from
unsigned int Person::chooseOtherSite(unsigned int _from) const {
    return randomSiteExcept(NBSITES, _from);
}

// Random travel time for bike (ms)
unsigned int Person::bikeTravelTime() const {
    return randomTravelTimeMs() + 1000; // add minimum time
}

// Random travel time for walking (ms)
unsigned int Person::walkTravelTime() const {
    return randomTravelTimeMs() + 2000; // add minimum time
}

// Log a message to the console (if GUI exists)
void Person::log(const QString& msg) const {
    if (binkingInterface) {
        binkingInterface->consoleAppendText(id, msg);
    }
}
