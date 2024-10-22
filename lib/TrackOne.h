//
// Created by ron0 on 20/10/24.
//
#include <array> // std::array
#include <stdint.h> // uint64_t


#ifndef TECHARENA2024_TRACKONE_H
#define TECHARENA2024_TRACKONE_H

namespace Arena {

    class TrackOne {
    public:
        TrackOne();
        ~TrackOne();

        // creates micro feedforward neural network
        void createNeuralNetwork();

    private:
        std::array<uint64_t, 4> params;

    };

} // Arena

#endif //TECHARENA2024_TRACKONE_H
