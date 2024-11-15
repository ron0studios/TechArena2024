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
#include <utility>



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
#define BIMODAL 16384

// Place your RoboMemory content here
// Note that the size of this data structure can't exceed 64KiB!
struct RoboPredictor::RoboMemory {
  static const std::uint32_t PRIME = 2654435769;

  // we will implement a 64, 256, 1024, and 16384 tage

  // the current number of planets we've gone through
  int progress = 0; // KEEP THIS UPDATED!
  bool pred = false;
   
  // main history 
  
  std::uint8_t hist8 = 0;
  std::uint16_t hist16 = 0;
  std::uint64_t hist64 = 0;
  std::array<__uint128_t,2> hist256 = {};
  std::array<__uint128_t,16> hist2048 = {};

  struct tag {
    std::uint8_t tag;
    std::uint8_t counter;
  };

  std::array<tag, BANK_S> bank1 = {};
  std::array<tag, BANK_S> bank2 = {};
  std::array<tag, BANK_S> bank3 = {};
  std::array<tag, BANK_S> bank4 = {};



  // implement bimodal
  struct Bimodal {
  // 20 bits planet id 10 bits usefulness 2 bits saturator
  std::array<uint32_t, BIMODAL> data = {};
  std::uint64_t hist = 0;
  int lru0 = 0, lru1 = 0;

  Bimodal(){
    //data.fill({UINT8_MAX, UINT8_MAX, UINT8_MAX});
    data.fill(0xFFFFF000);
  }

  // a knuth multiplicative hash between 0 and 2**p (default 4096)
  // its a terrible hash function but does the job (i think)
  std::uint16_t hash(std::uint64_t x, int p=14) {
    return ( (x*11400714819323198485ULL)>>(64-p) ) ;
  }

  // gets the first 20 bits of an array of 3 8 bit numbers
  std::uint64_t getid(std::array<std::uint8_t, 3> entry){
    return (entry[2]&0xF0)|(entry[1]<<8)|(entry[0]<<16);
  }

  std::uint64_t getid(std::uint32_t entry){
    return (entry&0xFFFFF000) >> 12;
  }

  // gets the last 4 bits of an array of 3 8 bit numbers
  std::uint8_t getval(std::array<std::uint8_t, 3> entry){
    return entry[2]&0x0F;
  }

  std::uint8_t getval(std::uint32_t entry){
    return entry&0x00000003;
  }

  std::uint16_t getuseful(std::uint32_t entry){
    return entry&0xFFFFF003;
  }

  int getentry(std::uint64_t planetid) {
    int i = 0;
    std::uint64_t ptr = getid(data[i]);
    while(ptr != planetid){
    int l = (i<<1) + 1;
    int r = l+1; 
    if(planetid < ptr) {
      if(l>=BIMODAL) return -1; 

      std::uint64_t lid = getid(data[l]);
      if(lid > 1000000) return -1;

      i = l;
    } else {
      if(r>=BIMODAL) return -1; 

      std::uint64_t rid = getid(data[r]);
      if(rid > 1000000) return -1;

      i = r;
    }
    ptr = getid(data[i]);
    }
    return i;
  }

  bool get(std::uint64_t planetid){
    int idx = getentry(planetid);

    // default false bcs 80% of the data is false
    if(idx == -1) return false;
    std::uint8_t val = getval(data[idx]);

    //std::cout << (unsigned long long)h << std::endl;
    if((val >> 1) % 2){
    return true;
    } else {
    return false;
    }
  }

  // preferably do this every once in a while since the tree
  // becomes filled with useless keys pretty often
  // TODO: replace this functionality with the usefulness counter
  void reset(){
    data.fill(0xFFFFFFFF);
  }

  void update(std::uint64_t planetid, bool outcome, bool pred){
    hist = (hist << 1);
    hist |= outcome;

    // option 1: look for planet if it exists
    int op1 = getentry(planetid);
    if(op1 != -1){
    std::uint8_t val = getval(data[op1]);
    //data[op1][2] |= (outcome) ? std::min(3, val+1) : std::max(0, val-1);
    data[op1] &= 0xFFFFFFFC;
    data[op1] |= (outcome) ? std::min(3, val+1) : std::max(0, val-1);

    return;
    }

    // option 2: create new node in position
    int i = 0;
    std::uint64_t ptr = getid(data[i]);
    if(ptr > 1000000){
    data[i] &= 0; // TODO: save counter as well!
    data[i] |= planetid<<12;
    return;
    }
    while(ptr != planetid){
    int l = (i<<1) + 1;
    int r = l+1; 
    if(planetid < ptr) {
      if(l>=BIMODAL) return; 

      std::uint64_t lid = getid(data[l]);
      if(lid > 1000000) {
      data[l] &= 0; // TODO: save counter as well!
      data[l] |= planetid<<12;
      return;
      };

      i = l;
    } else {
      if(r>=BIMODAL) return;

      std::uint64_t rid = getid(data[r]);
      if(rid > 1000000){
      data[r] &= 0; // TODO: save counter as well!
      data[r] |= planetid<<12;
      return;
      }

      i = r;
    }
    ptr = getid(data[i]);
    }


    //std::uint16_t h = hash(planetid);
    //data[h] = (outcome) ? std::min(3, data[h] + 1) : std::max(0, data[h] - 1);
  }






  };
  Bimodal bimodal;


  std::array<__uint128_t, 2> xor256(std::array<__uint128_t, 2> x, std::array<__uint128_t, 2> y){
    return {x[0]^y[0], x[1]^y[1]};}
  std::array<__uint128_t, 2> or256(std::array<__uint128_t, 2> x, std::array<__uint128_t, 2> y){
    return {x[0]|y[0], x[1]|y[1]};}
  std::array<__uint128_t, 2> and256(std::array<__uint128_t, 2> x, std::array<__uint128_t, 2> y){
    return {x[0]^y[0], x[1]^y[1]};}

  // a knuth multiplicative hash between 0 and 2**p (default 4096)
  std::uint64_t hash(std::uint64_t x, int p=12) {
    return (x*11400714819323198485ULL)>>(64-p);
  }


  bool get_pred(std::uint64_t planetid){
    pred = bimodal.get(planetid);
    return pred;
  }

  void updateall(std::uint64_t planetid, bool outcome){
    //std::cout << progress << std::endl;
    //addrecent(planetid);
    hist64 = (hist64 << 1);
    hist64 |= outcome;

    hist16 = hist16 << 1;
    hist16 |= outcome;

    //std::cout << std::bitset<8>(*std::max_element(bimodal.data.begin(), bimodal.data.end())) << "\t" << (int)planetid << "\t" << pred << "\t" << outcome << std::endl;
    bimodal.update(planetid, outcome, pred);
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
  //bool out = roboMemory_ptr->getbimodal(nextPlanetID);
  //roboMemory_ptr->pred = out;
  //std::cout << "\t" << out << std::endl;
  return roboMemory_ptr->get_pred(nextPlanetID);
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
// static_assert(
//     sizeof(RoboPredictor::RoboMemory) <= 65536,
//     "Robo's memory exceeds 65536 bytes (64KiB) in your implementation. "
//     "Prediction algorithms using so much "
//     "memory are ineligible. Please reduce the size of your RoboMemory struct.");


// Declare constructor/destructor for RoboPredictor
RoboPredictor::RoboPredictor() {
  std::cout << "Memory size: " << sizeof(RoboPredictor::RoboMemory) << std::endl;
  roboMemory_ptr = new RoboMemory;
}
RoboPredictor::~RoboPredictor() {
  delete roboMemory_ptr;
}
