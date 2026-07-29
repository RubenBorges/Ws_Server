#pragma once

#include <iostream>
#include <random>
#include <type_traits>
#include <concepts>     
#include <algorithm> // For std::min and std::max

namespace bpy::utility {

// 1. Constrain T using concepts to be either purely integral OR floating_point
template <typename T>
requires std::integral<T> || std::floating_point<T>
T getRandomNumber(T x, T y) {
    // 2. Initialize a random device to obtain a hardware seed
    static std::random_device rd;
    
    // 3. Seed a standard pseudo-random engine (Mersenne Twister) once
    static std::mt19937 gen(rd());

    // 4. Ensure the range is correctly ordered from low to high
    T low = std::min(x, y);
    T high = std::max(x, y);

    // 5. Compile-time branching based on numeric category
    if constexpr (std::floating_point<T>) {
        std::uniform_real_distribution<T> dist(low, high);
        return dist(gen);
    } else {
        std::uniform_int_distribution<T> dist(low, high);
        return dist(gen);
    }
}

} // namespace bpy::utility

// int main() {
//     // Pick an integer between 1 and 100 (inclusive)
//     int randomInt = getRandomNumber(1, 100);
//     std::cout << "Random Int: " << randomInt << "\n";

//     // Pick a float between 1.5 and 9.5 (inclusive)
//     double randomDouble = getRandomNumber(1.5, 9.5);
//     std::cout << "Random Double: " << randomDouble << "\n";

//     return 0;
// }
