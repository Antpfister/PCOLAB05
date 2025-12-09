/*
 * Auteur : Anthony Pfister, Santiago Sugranes
 * Date : 09.12.2025
 * PCO2025 lab05
 * HEIG
 */



#include "van.h"

// Initialize static members
BikingInterface* Van::binkingInterface = nullptr; // pointer to GUI / interface
std::array<BikeStation*, NB_SITES_TOTAL> Van::stations{}; // all bike stations
bool Van::stopVanRequested = false; // flag to request van stop

// Constructor: sets van ID and initial site (depot)
Van::Van(unsigned int _id)
    : id(_id),
      currentSite(DEPOT_ID)
{}

// Main van loop
void Van::run() {
    while (!stopVanRequested) { // keep running until stop requested
        loadAtDepot(); // load some bikes at the depot

        // Visit each site to balance bikes
        for (unsigned int s = 0; s < NBSITES; ++s) {
            driveTo(s);       // drive to the site
            balanceSite(s);   // balance bikes at the site
        }

        returnToDepot(); // return to depot to unload
    }

    log("Van stops cleanly"); // log message when loop ends
}

// Set the GUI / interface pointer
void Van::setInterface(BikingInterface* _binkingInterface){
    binkingInterface = _binkingInterface;
}

// Set the stations shared by all vans
void Van::setStations(const std::array<BikeStation*, NB_SITES_TOTAL>& _stations) {
    stations = _stations;
}

// Log a message to the console or GUI
void Van::log(const QString& msg) const {
    if (binkingInterface) {
        binkingInterface->consoleAppendText(0, msg);
    }
}

// Drive to a destination site
void Van::driveTo(unsigned int _dest) {
    if (currentSite == _dest)
        return; // already at destination

    unsigned int travelTime = randomTravelTimeMs(); // random travel time
    if (binkingInterface) {
        binkingInterface->vanTravel(currentSite, _dest, travelTime); // GUI animation
    }

    currentSite = _dest; // update current site
}

// Load bikes at the depot into the van
void Van::loadAtDepot() {
    driveTo(DEPOT_ID); // make sure we're at the depot
    cargo.clear();      // empty the van cargo

    BikeStation* depot = stations[DEPOT_ID];

    // Load at most min(2, D) bikes
    size_t D = depot->nbBikes(); // number of bikes available
    size_t toLoad = std::min((size_t)2, D);

    if (toLoad > 0) {
        // getBikes respects FIFO and wakes up waiting threads
        std::vector<Bike*> loaded = depot->getBikes(toLoad);
        for (Bike* b : loaded) {
            cargo.push_back(b); // add to cargo
        }
    }

    // Update GUI to reflect new bike count at depot
    if (binkingInterface) {
        binkingInterface->setBikes(DEPOT_ID, stations[DEPOT_ID]->nbBikes());
    }
}

// Balance bikes at a specific site
void Van::balanceSite(unsigned int _site)
{
    if (_site == DEPOT_ID) return; // skip the depot

    BikeStation* st = stations[_site];

    unsigned int target = BORNES - 2; // target number of bikes per site
    unsigned int Vi = st->nbBikes();   // current bikes at the site
    unsigned int a = cargo.size();     // current bikes in the van

    //
    // ===== CASE 1: SURPLUS → remove bikes from the site =====
    //
    if (Vi > target) {
        unsigned int surplus = Vi - target;           // number of bikes to remove
        unsigned int freeSpace = VAN_CAPACITY - a;   // free space in the van
        unsigned int c = std::min(surplus, freeSpace);

        if (c > 0) {
            std::vector<Bike*> taken = st->getBikes(c); // take c bikes
            for (Bike* b : taken)
                cargo.push_back(b); // add to cargo
        }
    }

    //
    // ===== CASE 2: DEFICIT → deposit bikes =====
    //
    else if (Vi < target) {
        unsigned int needed = target - Vi;            // number of bikes to add
        unsigned int a = cargo.size();
        unsigned int c = std::min(needed, a);         // number of bikes we can deposit

        std::vector<Bike*> toAdd;
        toAdd.reserve(c);
        unsigned int deposited = 0;

        //
        // 2b.1 — deposit one bike of each missing type first
        //
        for (size_t t = 0; t < Bike::nbBikeTypes && deposited < c; ++t) {
            if (st->countBikesOfType(t) == 0) {      // type is missing
                Bike* chosen = takeBikeFromCargo(t); // take bike of this type from cargo
                if (chosen) {
                    toAdd.push_back(chosen);        // prepare to deposit
                    deposited++;
                }
            }
        }

        //
        // 2b.2 — fill remaining slots with any bikes left in the cargo
        //
        while (deposited < c && !cargo.empty()) {
            Bike* b = cargo.back();
            cargo.pop_back();
            toAdd.push_back(b);
            deposited++;
        }

        //
        // Deposit the bikes into the station
        //
        if (!toAdd.empty()) {
            std::vector<Bike*> rejected = st->addBikes(toAdd); // bikes that could not be added

            // Return rejected bikes to cargo
            for (Bike* b : rejected)
                cargo.push_back(b);
        }
    }

    // Update GUI to reflect new bike count at site and depot
    if (binkingInterface) {
        binkingInterface->setBikes(_site, st->nbBikes());
        binkingInterface->setBikes(DEPOT_ID, stations[DEPOT_ID]->nbBikes());
    }
}

// Return to depot and unload cargo
void Van::returnToDepot() {
    driveTo(DEPOT_ID);

    BikeStation* depot = stations[DEPOT_ID];

    if (!cargo.empty()) {
        // addBikes can return bikes if simulation ended
        std::vector<Bike*> rejected = depot->addBikes(cargo);

        // If none could be added → shutdown detected
        if (rejected.size() == cargo.size()) {
            stopVanRequested = true;
            return; // run() will detect stopVanRequested and exit
        }

        cargo.clear(); // clear cargo
    }

    // Update GUI
    if (binkingInterface) {
        binkingInterface->setBikes(DEPOT_ID, stations[DEPOT_ID]->nbBikes());
    }
}

// Take a bike of a specific type from the cargo
Bike* Van::takeBikeFromCargo(size_t type) {
    for (size_t i = 0; i < cargo.size(); ++i) {
        if (cargo[i]->bikeType == type) {
            Bike* bike = cargo[i];
            cargo[i] = cargo.back(); // swap with last bike for fast removal
            cargo.pop_back();
            return bike;
        }
    }
    return nullptr; // no bike of this type in cargo
}
