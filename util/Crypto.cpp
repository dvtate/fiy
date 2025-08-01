//
// Created by tate on 7/31/25.
//

#include "Crypto.hpp"

std::mt19937& Crypto::rng() {
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    return gen;
}