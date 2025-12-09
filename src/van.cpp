#include "van.h"

BikingInterface* Van::binkingInterface = nullptr;
std::array<BikeStation*, NB_SITES_TOTAL> Van::stations{};

bool Van::stopVanRequested = false;

Van::Van(unsigned int _id)
    : id(_id),
    currentSite(DEPOT_ID)
{}


void Van::run() {
     while (!stopVanRequested) {
        loadAtDepot();
        for (unsigned int s = 0; s < NBSITES; ++s) {
            driveTo(s);
            balanceSite(s);
        }
        returnToDepot();
    }
    log("Van s'arrête proprement");
}

void Van::setInterface(BikingInterface* _binkingInterface){
    binkingInterface = _binkingInterface;
}

void Van::setStations(const std::array<BikeStation*, NB_SITES_TOTAL>& _stations) {
    stations = _stations;
}

void Van::log(const QString& msg) const {
    if (binkingInterface) {
        binkingInterface->consoleAppendText(0, msg);
    }
}

void Van::driveTo(unsigned int _dest) {
    if (currentSite == _dest)
        return;

    unsigned int travelTime = randomTravelTimeMs();
    if (binkingInterface) {
        binkingInterface->vanTravel(currentSite, _dest, travelTime);
    }

    currentSite = _dest;
}

void Van::loadAtDepot() {
    driveTo(DEPOT_ID);

    cargo.clear();

    BikeStation* depot = stations[DEPOT_ID];

    // A = min(2, D)
    size_t D = depot->nbBikes();
    size_t toLoad = std::min((size_t)2, D);

    if (toLoad > 0) {
        // getBikes respecte l'ordre FIFO et réveille les bons threads
        std::vector<Bike*> loaded = depot->getBikes(toLoad);

        for (Bike* b : loaded) {
            cargo.push_back(b);
        }
    }

    if (binkingInterface) {
        binkingInterface->setBikes(DEPOT_ID, stations[DEPOT_ID]->nbBikes());
    }
}


void Van::balanceSite(unsigned int _site)
{
    if (_site == DEPOT_ID) return;

    BikeStation* st = stations[_site];

    unsigned int target = BORNES - 2;   // objectif : B − 2 vélos à chaque site
    unsigned int Vi = st->nbBikes();
    unsigned int a = cargo.size();               // capacité actuelle dans la camionnette

    // vélos dans la camionnette

    //
    // ====== CAS 1 : SURPLUS → enlever des vélos du site ======
    //
    if (Vi > target) {
        unsigned int surplus = Vi - target;
        unsigned int freeSpace = VAN_CAPACITY - a;
        unsigned int c = std::min(surplus, freeSpace);

        if (c > 0) {
            std::vector<Bike*> taken = st->getBikes(c);
            for (Bike* b : taken)
                cargo.push_back(b);
        }
    }

    //
    // ====== CAS 2 : MANQUE → déposer des vélos ======
    //
    else if (Vi < target) {
        unsigned int needed = target - Vi;
        unsigned int a = cargo.size();
        unsigned int c = std::min(needed, a);

        std::vector<Bike*> toAdd;
        toAdd.reserve(c);
        unsigned int deposited = 0;
        //
        // 2b.1 — Déposer en priorité 1 vélo de CHAQUE TYPE MANQUANT
        //
        for (size_t t = 0; t < Bike::nbBikeTypes && deposited < c; ++t) {
            // TYPE MANQUANT ? (conformément au cahier des charges)
            if (st->countBikesOfType(t) == 0) {
                Bike* chosen = takeBikeFromCargo(t);
                if (chosen) {
                    toAdd.push_back(chosen);
                    deposited++;
                }
            }
        }
        //
        // 2b.2 — Compléter avec n’importe quels vélos restants dans la camionnette
        //
        while (deposited < c && !cargo.empty()) {
            Bike* b = cargo.back();
            cargo.pop_back();
            toAdd.push_back(b);
            deposited++;
        }

        //
        // Dépôt des vélos dans le moniteur
        //
        if (!toAdd.empty()) {
            std::vector<Bike*> rejected = st->addBikes(toAdd);

            // Les vélos non déposés (fin de simulation) retournent dans le cargo
            for (Bike* b : rejected)
                cargo.push_back(b);
        }
    }

    if (binkingInterface) {
        binkingInterface->setBikes(_site, st->nbBikes());
        binkingInterface->setBikes(DEPOT_ID, stations[DEPOT_ID]->nbBikes());
    }
}

void Van::returnToDepot() {
    driveTo(DEPOT_ID);

    BikeStation* depot = stations[DEPOT_ID];

    if (!cargo.empty()) {
        // addBikes peut renvoyer certains vélos si shouldEnd est true
        std::vector<Bike*> rejected = depot->addBikes(cargo);

        // Les vélos qui n'ont pas pu être ajoutés doivent rester dans le cargo
        if (rejected.size() == cargo.size()) {
            // Mode shutdown détecté
            stopVanRequested = true;
            return; // Le run() détectera stopVanRequested et sortira de la boucle
        }

        cargo.clear();
    }

    if (binkingInterface) {
        binkingInterface->setBikes(DEPOT_ID, stations[DEPOT_ID]->nbBikes());
    }
}

Bike* Van::takeBikeFromCargo(size_t type) {
    for (size_t i = 0; i < cargo.size(); ++i) {
        if (cargo[i]->bikeType == type) {
            Bike* bike = cargo[i];
            cargo[i] = cargo.back();
            cargo.pop_back();
            return bike;
        }
    }
    return nullptr;
}

