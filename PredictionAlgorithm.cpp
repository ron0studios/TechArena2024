// This file contains a template for the implementation of Robo prediction
// algorithm

#include "PredictionAlgorithm.hpp"
#include <array>
#include <bitset>
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <iostream>

// Place your RoboMemory content here
// Note that the size of this data structure can't exceed 64KiB!
struct RoboPredictor::RoboMemory {
  #define RECENCY 256
  #define BITHISTORY 4096
  #define GLOBALHISTORY 16
  #define WEIGHTS 33
  const std::uint32_t PRIME = 2654435769;
  const std::uint64_t KNUTH = 11400714819323198485;

  // the current number of planets we've gone through
  int progress = 0; // KEEP THIS UPDATED!

  std::uint64_t history = 0;

  // a set of perceptron weights mapped by an LRU
  // FORMAT OF THE NN:
  // - planet id
  // - last 16 bits history
  // - last 16 bits used by planet
  // 33 total
  std::unordered_map<std::uint64_t, std::array<std::int8_t, WEIGHTS>> NN;
  std::array<std::uint16_t, BITHISTORY> nlru;
    int nlru0 = 0; // front of queue
    int nlru1 = 0; // back of queue
  

  // an ultra-simple LRU cache for recency
  // OLD std::unordered_map<std::uint64_t, int> recency;
  std::array<std::uint64_t, RECENCY*2> recency; // 2x load factor
  std::array<std::uint64_t, RECENCY> rlru;
    int rlru0 = 0; // front of queue
    int rlru1 = 0; // back of queue

  // an ultra-simple LRU cache for the "bit history" of a planet
  //std::unordered_map<std::uint64_t, std::uint16_t> bithist;
  std::array<std::uint16_t, BITHISTORY*2> bithist; // 2x load factor
  std::array<std::uint16_t, BITHISTORY> blru;
    int blru0 = 0; // front of queue
    int blru1 = 0; // back of queue
  
  RoboMemory(){
    recency.fill(0);
    bithist.fill(0);
    //recency.reserve(RECENCY+10); // arbitrary +10
    //bithist.reserve(BITHISTORY+10); // arbitrary +10
    NN.reserve(BITHISTORY+10); // arbitrary +10
  }

  // a knuth multiplicative hash between 0 and 2**p (default 4096)
  std::uint64_t hash(std::uint64_t x, int p=12) {
    return (x*KNUTH) >> (64-p);
  }

  void addrecent(std::uint64_t planetid){
    std::uint64_t h = hash(planetid, 9);
    //std::cout << h << std::endl;
    if(rlru0 == rlru1){
      //recency.erase(rlru[rlru0]);
      recency[h] = 0;
      rlru0 = (rlru0 + 1) % RECENCY;
    }

    rlru[rlru1] = planetid;
    recency[h] = progress;
    rlru1 = (rlru1 + 1) % RECENCY;
  }

  int getrecent(int planetid){
    std::uint64_t h = hash(planetid, 9);
    //if(!recency.count(planetid))
    if(!recency[h])
     return RECENCY;

    return progress-recency[h];
  }


  void addbithist(std::uint64_t planetid, bool outcome){
    std::uint64_t h = hash(planetid, 13); // 12+1 for load factor
    if(blru0 == blru1){
      //bithist.erase(blru[blru0]);
      bithist[h] = 0;
      NN.erase(blru[blru0]);
      blru0 = (blru0 + 1) % BITHISTORY;
    }

    blru[blru1] = planetid;
    bithist[h] = (bithist[h] << 1) | outcome;
    blru1 = (blru1 + 1) % BITHISTORY;
  }

  std::uint64_t getbithist(std::uint64_t planetid){
    if(!bithist.count(planetid))
     return 0;

    return bithist[planetid];
  }

  void updateNN(std::uint64_t planetid, bool outcome){
    if(!NN.count(planetid)){
      NN[planetid].fill(123);
    }

  }



  void updateall(std::uint64_t planetid, bool outcome){
    addbithist(planetid, outcome);
    addrecent(planetid);
    //updateNN(planetid, outcome);
    history = (history << 1) | outcome;
  }
};

// Robo can consult data structures in its memory while predicting.
// Example: access Robo's memory with roboMemory_ptr-><your RoboMemory
// content>

// Robo can perform computations using any data in its memory during
// prediction. It is important not to exceed the computation cost threshold
// while making predictions and updating RoboMemory. The computation cost of
// prediction and updating RoboMemory is calculated by the playground
// automatically and printed together with accuracy at the end of the
// evaluation (see main.cpp for more details).
//
// THIS RUNS FIRST
bool RoboPredictor::predictTimeOfDayOnNextPlanet(
    std::uint64_t nextPlanetID, bool spaceshipComputerPrediction) {

  // get recency
  int recency = roboMemory_ptr->getrecent(nextPlanetID);

  //std::cout << nextPlanetID << "\t" << recency << std::endl;

  return spaceshipComputerPrediction;
}

// Robo can consult/update data structures in its memory
// Example: access Robo's memory with roboMemory_ptr-><your RoboMemory
// content>

// It is important not to exceed the computation cost threshold while making
// predictions and updating RoboMemory. The computation cost of prediction and
// updating RoboMemory is calculated by the playground automatically and
// printed together with accuracy at the end of the evaluation (see main.cpp
// for more details).
//
// THIS RUNS SECOND
void RoboPredictor::observeAndRecordTimeofdayOnNextPlanet( std::uint64_t nextPlanetID, bool timeOfDayOnNextPlanet) {

  // update recency
  
  roboMemory_ptr->updateall(nextPlanetID, timeOfDayOnNextPlanet);
  roboMemory_ptr->progress++;
}


//------------------------------------------------------------------------------
// Please don't modify this file below
//
// Check if RoboMemory does not exceed 64KiB
static_assert(
    sizeof(RoboPredictor::RoboMemory) <= 65536,
    "Robo's memory exceeds 65536 bytes (64KiB) in your implementation. "
    "Prediction algorithms using so much "
    "memory are ineligible. Please reduce the size of your RoboMemory struct.");

// Declare constructor/destructor for RoboPredictor
RoboPredictor::RoboPredictor() {
  std::cout << "Memory size: " << sizeof(RoboPredictor::RoboMemory) << std::endl;
  roboMemory_ptr = new RoboMemory;
}
RoboPredictor::~RoboPredictor() {
  delete roboMemory_ptr;
}
