//
// Created by ron0 on 22/10/24.
//
// This file contains utility functions that are used across the project.
#include <stdint.h> // uint8_t

#ifndef TECHARENA2024_UTILS_H
#define TECHARENA2024_UTILS_H

#endif //TECHARENA2024_UTILS_H

namespace Arena::Utils {
    // returns the number of bits set in a byte
    // copilot is waffling here...
    unsigned char countBits(unsigned char byte) {
        unsigned char count = 0;
        while (byte) {
            count += byte & 1;
            byte >>= 1;
        }
        return count;
    }

    inline uint8_t applyMat(){
        return 0;
    }

} // Arena::Utils