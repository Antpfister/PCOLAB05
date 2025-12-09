# Rapport sur la gestion concurrente du système de vélos

**Auteur :** Anthony Pfister, Santiago Sugranes  
**Date :** 09.12.2025  
**Cours :** PCO2025 lab05, HEIG  

---

## 1. Contexte et choix de conception

Le système simule une application de vélos en libre-service avec :  

- Des **stations de vélos (`BikeStation`)** qui gèrent les vélos par type et avec une capacité limitée.  
- Des **utilisateurs (`Person`)** qui prennent et déposent des vélos en continu.  
- Une **interface graphique (`BikingInterface`)** pour suivre le nombre de vélos et les déplacements.  
- Optionnellement, un **camion (`Van`)** pour équilibrer les vélos entre les stations.  

**Choix principaux :**  

1. **Mutex + condition variables** pour synchroniser l’accès aux vélos dans `BikeStation`.  
2. **Style Mesa** pour les boucles d’attente (`while`) afin de garantir que les threads se réveillent correctement.  
3. **Variables thread-local** pour le générateur aléatoire (`rng`) dans `Person` afin de prévenir les conflits entre threads.  
4. Les variables **partagées globales** (`stations`, `binkingInterface`) sont accessibles par tous les threads.  

---

## 2. Analyse des problèmes de concurrence

### 2.1 BikeStation

- Les méthodes `getBike`, `putBike`, `addBikes`, `getBikes` utilisent toutes un **mutex pour protéger les accès aux données partagées**. 
- Les condition variables `condPutters` et `condTakers` sont utilisées avec des boucles `while` pour le style Mesa. 
- Le drapeau `shouldEnd` est protégé par le mutex et testé avant et après les attentes, ce qui empêche les deadlocks. 
- **Remarques mineures :**
  - Les lectures via `countBikesOfType` et `nbBikes()` sont cohérentes au moment de l’appel mais peuvent devenir obsolètes après unlock.  
  - Certaines notifications (`notifyOne`) peuvent réveiller des threads inutiles, mais cela ne cause pas de crash.  

### 2.2 Person

- Chaque `Person` a ses **variables propres** (`id`, `currentSite`, `preferredType`) pas de conflit. 
- Les interactions avec les stations passent par `BikeStation` sécurisées par mutex. 


### 2.3 Van

- La classe `Van` est **multi-thread safe** pour les interactions avec les stations grâce aux méthodes protégées par mutex de `BikeStation`.
- Le cargo interne (`std::vector<Bike*> cargo`) est propre au thread `Van` pas de conflit entre threads.
- Les variables statiques partagées (`stations`, `binkingInterface`, `stopVanRequested`) :
    - `stations` et `binkingInterface` doivent être **initialisées avant le lancement des threads**.
    - `stopVanRequested` est un bool utilisé pour arrêter le thread `Van` **pas atomique**, mais il n’y a pas de write concurrent puisque seul le thread Van le modifie, donc safe dans ce contexte.
- Les fonctions `balanceSite` et `returnToDepot` respectent le **pattern Mesa** via les méthodes `getBikes` et `addBikes`.
- Aucune modification directe des `BikeStation::bikesByType` en dehors des fonctions thread-safe → pas de corruption de données.


## 3. Tests effectués

1. **Simulation multi-threads** avec plusieurs `Person` :  
   - Tous les vélos pris et déposés correctement.  
   - Aucune perte de vélo détectée.  

2. **Fin de simulation** :  
   - Les threads `Person` terminent correctement quand `shouldEnd` est activé.  
   - Les vélos non déposés restent dans le cargo ou sont renvoyés correctement.  

3. **Stress test** :  
   - Plusieurs threads `Person` prennent et déposent en même temps sur la même station.  
   - Aucune corruption de `bikesByType` détectée.
---


