// This file contains a template for the implementation of Robo prediction
// algorithm

#include "PredictionAlgorithm.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <bitset>
#include <stdio.h>

// Place your RoboMemory content here
// Note that the size of this data structure can't exceed 64KiB!
struct RoboPredictor::RoboMemory {
  #define RECENCY 256
  #define BITHISTORY 1024
  #define WEIGHTS_HIST 16
  #define WEIGHTS_BITS 0
  #define LEARNING_THRESHOLD 128
  #define GSHARE 256
  #define LOCPRED 4096 // local prediction
  #define BANK1 8
  #define BANK2 32
  #define BANK3 64
  #define BANK4 128
  #define BANK_S 256
  #define BANK_L 2048
  #define BIMODAL 32768
  const std::uint32_t PRIME = 2654435769;
  const std::uint64_t KNUTH = 11400714819323198485;

  // we will implement a 64, 256, 1024, and 16384 tage

  // the current number of planets we've gone through
  int progress = 0; // KEEP THIS UPDATED!
  bool pred = false;
   
  // main history 
  
  std::uint8_t hist8 = 0;
  std::uint16_t hist16 = 0;
  std::uint64_t hist64 = 0;
  std::array<__uint128_t,2> hist256 = {};


  std::array<std::uint16_t, BANK_S> bank1 = {};
  std::array<std::uint16_t, BANK_S> bank2 = {};
  std::array<std::uint16_t, BANK_S> bank3 = {};
  std::array<std::uint16_t, BANK_S> bank4 = {};

  // implement gshare
  std::array<std::uint8_t, GSHARE> gshare = {};
  std::array<std::uint8_t, 32768> bimodal = {};



  std::array<__uint128_t, 2> lshift256(std::array<__uint128_t, 2> x){
    x[0] <<= 1;
    x[0] |= !std::min(1,__builtin_clz(x[1]));
    x[1] <<= 1;
    return x;
  }

  std::array<__uint128_t, 2> rshift256(std::array<__uint128_t, 2> x){
    x[0] <<= 1;
    x[0] |= !std::min(1,__builtin_clz(x[1]));
    x[1] <<= 1;
    return x;
  }

  std::array<__uint128_t, 2> xor256(std::array<__uint128_t, 2> x, std::array<__uint128_t, 2> y){
    return {x[0]^y[0], x[1]^y[1]};
  }

  std::array<__uint128_t, 2> or256(std::array<__uint128_t, 2> x, std::array<__uint128_t, 2> y){
    return {x[0]|y[0], x[1]|y[1]};
  }

  std::array<__uint128_t, 2> and256(std::array<__uint128_t, 2> x, std::array<__uint128_t, 2> y){
    return {x[0]^y[0], x[1]^y[1]};
  }

  // a knuth multiplicative hash between 0 and 2**p (default 4096)
  std::uint64_t hash(std::uint64_t x, int p=12) {
    return (x*KNUTH) >> (64-p);
  }




  bool getbimodal(std::uint64_t planetid){
    std::uint16_t h = hash(planetid, 15);
    //std::uint64_t h = planetid;
    if(bimodal[h] > 4){
      return true;
    } else return false;
  }

  void addbimodal(std::uint64_t planetid, bool outcome){
    std::uint16_t h = hash(planetid, 15);
    //std::uint64_t h = planetid;
    if(pred==outcome){
      bimodal[h] = (pred) ? std::min(8, bimodal[h]+1) : std::max(0, bimodal[h]-1);
    } else {
      bimodal[h] = (pred) ? std::max(0, bimodal[h]-1) : std::min(8, bimodal[h]+1);
    }
  }

  /* * * * * * * * * * * * * * *
  * |-------------------------| *
  * | HASHING FOR pseudo-TAGE | *
  * |-------------------------| *
  * * * * * * * * * * * * * * * */
  std::uint16_t tag_hash(std::uint64_t planetid, std::uint64_t hist){
    return 0;
  }


  // a gshare hash XOR's both the GHR and planetid then applies
  // knuths hash to bring the hash range down to 14 bita.
  // assumes planetid is roughly maxes at 16 bit range
  std::uint16_t gshare_hash(std::uint64_t planetid, std::uint64_t hist){
    return foldhist(hist) ^ (std::uint16_t)hash(planetid,16);
    //return hash(foldhist(hist64) ^ (std::uint16_t)hash(planetid,16), 14);
  }

  std::uint16_t gshare_hash16(std::uint64_t planetid){
    return hist16 ^ (std::uint16_t)hash(planetid,16);
    //return hash(foldhist(hist64) ^ (std::uint16_t)hash(planetid,16), 14);
  }

  // gets the 2 bit data from a hashed 16 bit id
  std::uint8_t gshare_retrieve(std::uint16_t idx){
    std::uint16_t tableid = idx >> 2;
    std::uint16_t rowid = (idx&1) | (idx&2);
    std::uint16_t val = (gshare[tableid] >> (rowid << 1)) & 3;
    return val;
  }
  
  // modified the stored 2 bit data from a hashed 16 bit id
  void gshare_replace(std::uint16_t idx, std::uint8_t newval){
    std::uint16_t tableid = idx >> 2;

    //std::cout << std::bitset<8>(gshare[tableid]) << "\t" << std::bitset<8>(newval) << std::endl;
    std::uint16_t rowid = (idx&1) | (idx&2);
    gshare[tableid] = (newval%2) ? gshare[tableid] | 1<<(2*rowid) 
      : gshare[tableid] & ~(1<<(2*rowid));
    gshare[tableid] = ((newval>>1)%2) ? gshare[tableid] | 1<<(2*rowid+1) 
      : gshare[tableid] & ~(1<<(2*rowid+1));
  }

  // folds a 64 bit history into 16 bit number
  std::uint16_t foldhist(std::uint64_t hist){
    std::uint16_t out = hist;
    out ^= hist >> 16;
    out ^= hist >> 32;
    out ^= hist >> 48;
    return out;
  }

  // an ultra-simple LRU cache for recency
  // OLD std::unordered_map<std::uint64_t, int> recency;
  std::array<std::uint8_t, RECENCY*2> recency; // 2x load factor
  std::array<std::uint16_t, RECENCY> rlru;
    int rlru0 = 0; // front of queue
    int rlru1 = 0; // back of queue

  
  RoboMemory(){
    recency.fill(0);
    bimodal.fill(4);
  }


  void addrecent(std::uint64_t planetid){
    std::uint64_t h = hash(planetid, 9);
    if(rlru0 == rlru1){
      //recency.erase(rlru[rlru0]);
      recency[rlru[rlru0]] = 0;
      rlru0 = (rlru0 + 1) % RECENCY;
    }

    rlru[rlru1] = h;
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

  bool getgshare(std::uint64_t planetid){
    std::uint16_t h = gshare_retrieve(gshare_hash16(planetid));

    if((h >> 1) % 2){
      return true;
    } else {
      return false;
    }
  }

  void updateall(std::uint64_t planetid, bool outcome){
    //std::cout << progress << std::endl;
    //addrecent(planetid);
    hist64 = (hist64 << 1);
    hist64 |= outcome;

    hist16 = hist16 << 1;
    hist16 |= outcome;

    addbimodal(planetid, outcome);

    //std::cout << (int)h << std::endl;
    //std::cout << (int)h << std::endl;
    //std::cout << std::bitset<64>(hist64) << std::endl;
    //std::cout << (int)foldhist(hist64) << std::endl;
    //std::cout <<  std::bitset<16>(hash(planetid,16)) << std::endl;
    //std::cout << std::bitset<64>(h) << std::endl;

    //std::cout << pred << std::endl;

    //if(pred) std::cout << "HOOO" << std::endl;

    std::uint16_t h = gshare_hash16(planetid);
    if(pred==outcome){
      //std::cout << "\tpred==outcome " << pred << std::endl;
      std::uint8_t newval = gshare_retrieve(h);
      newval = (pred) ? std::min(2,newval+1) : std::max(0, newval-1);
      //std::cout << "\t" << std::bitset<8>(newval) << std::endl;
      gshare_replace(h, newval);
    } else {
      std::uint8_t newval = gshare_retrieve(h);
      newval = (pred) ? std::max(0,newval-1) : std::min(2, newval+1);
      gshare_replace(h, newval);
    }

    //gshare_hash(planetid);
    //foldhist(hist64);
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
  //int recency = roboMemory_ptr->getrecent(nextPlanetID);
  //bool nn = roboMemory_ptr->getNN(nextPlanetID);

  //std::cout << nextPlanetID << "\t" << recency << std::endl;
  //std::cout << nextPlanetID << "\t" << std::bitset<16>(bithistory) << std::endl;
  bool out = roboMemory_ptr->getbimodal(nextPlanetID);
  roboMemory_ptr->pred = out;
  //std::cout << "\t" << out << std::endl;
  return out;//roboMemory_ptr->chosen_pred;
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
  //std::cout << "PREDICTED: " << roboMemory_ptr->myprediction << "\t" << "REAL: " << timeOfDayOnNextPlanet << std::endl;
  
  roboMemory_ptr->updateall(nextPlanetID, timeOfDayOnNextPlanet);
  roboMemory_ptr->progress++;


  // gshare test
}


//------------------------------------------------------------------------------
// Please don't modify this file below
//
// Check if RoboMemory does not exceed 64KiB
/*
static_assert(
    sizeof(RoboPredictor::RoboMemory) <= 65536,
    "Robo's memory exceeds 65536 bytes (64KiB) in your implementation. "
    "Prediction algorithms using so much "
    "memory are ineligible. Please reduce the size of your RoboMemory struct.");
*/


// Declare constructor/destructor for RoboPredictor
RoboPredictor::RoboPredictor() {
  std::cout << "Memory size: " << sizeof(RoboPredictor::RoboMemory) << std::endl;
  roboMemory_ptr = new RoboMemory;
}
RoboPredictor::~RoboPredictor() {
  delete roboMemory_ptr;
}
