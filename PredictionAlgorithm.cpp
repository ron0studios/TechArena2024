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

// Place your RoboMemory content here
// Note that the size of this data structure can't exceed 64KiB!
struct RoboPredictor::RoboMemory {
  static const std::uint32_t PRIME = 2654435769;
  static const std::uint64_t KNUTH = 11400714819323198485;

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
  // NOTE FOR TOMORROW
  // THE HASH COLLISION DOESNT WORK BECAUSE
  // THE CODE DOESNT KNOW WHEN TO STOP TRAVERSING
  // THE LINKED LIST BECAUSE THERE IS NO INDICATOR THAT
  // THE CURRENT NODE IS ACTUALLY THE VALUE WE WANT
  //
  // ALSO ALLOCATING 5 BITS TO THE POINTER MEANS A RANGE OF 32
  // NOT 256!!!
  struct Bimodal {
    // first 5 bits pointer, next 3 bits saturator
    std::array<std::uint8_t, 16384> data;

    Bimodal(){
      data.fill(0b11111111);
    }

    // a knuth multiplicative hash between 0 and 2**p (default 4096)
    // its a terrible hash function but does the job (i think)
    std::uint64_t hash(std::uint64_t x, int p=14) {
      return (x*KNUTH) >> (64-p);
    }

    std::uint16_t _get_idx(std::uint64_t planetid){
      std::uint16_t h = hash(planetid);
      std::uint8_t ptr = data[h];
      if(ptr>>3 == 0b11111) return 0b1111111111111111; // useless

      std::uint16_t i = h; 
      while(data[i]>>3 != 0){
        i = std::clamp(h-128+(data[i]>>3), 0, 16384-1);
      }

      return i;
    }

    std::uint8_t _get_val(std::uint64_t planetid){
      std::uint16_t idx =_get_idx(planetid); 
      if(idx == 0b1111111111111111){
        return 0b11111111;
      }
      return data[_get_idx(planetid)] & 0b00000111;
    }

    void _put_val(std::uint64_t planetid, std::uint8_t val){
      std::uint16_t h = hash(planetid);
      std::uint8_t ptr = data[h];
      if(ptr>>3 == 0b11111) data[h] = 0 | val;
      else {
        // h+126 because we reserve 0b11111 as NULL
         for(std::uint16_t i = std::max(h-128, 0); i < std::min(h+126, 16384-1); i++){
          if(data[i]>>3 == 0b11111){
            data[h] = (data[h] & 0b00000111) | ( (i-std::max(h-128,0)) << 3);
            data[i] = 0 | val;
            return;
          }
        }
        // wth why are you here...
        std::cout << "uh oh..." << std::endl;
      }
    }
    
    bool get(std::uint64_t planetid){
      std::uint8_t code = _get_val(planetid);
      if(code >> )
      if((code >> 2)%2){
        return true;
      } else {
        return false;
      }
    }

    void update(std::uint64_t planetid, bool outcome, bool pred){
      std::uint8_t cur = _get_val(planetid);
      _put_val(planetid, std::clamp( (outcome) ? cur+1: cur-1, 0, 7));
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
    return (x*KNUTH) >> (64-p);
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

    std::cout << std::bitset<8>(bimodal.data[0]) << "\t" << (int)planetid << "\t" << outcome<< std::endl;
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
  return spaceshipComputerPrediction; // out;//roboMemory_ptr->chosen_pred;
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
  std::cout << roboMemory_ptr->progress++ << std::endl;


  // gshare test
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
