#include <iostream>
#include <random>
#include <type_traits>

namespace bpy::utility{
template <typename T>
T getRandomNumber(T x, T y) {
    // 1. Initialize a random device to obtain a hardware seed
    static std::random_device rd;
    
    // 2. Seed a standard pseudo-random engine (Mersenne Twister) once
    static std::mt19937 gen(rd());

    // 3. Ensure the range is correctly ordered from low to high
    T low = std::min(x, y);
    T high = std::max(x, y);

    // 4. Use the correct distribution type based on the data type
    if constexpr (std::is_floating_point_v<T>) {
        std::uniform_real_distribution<T> dist(low, high);
        return dist(gen);
    } else {
        std::uniform_int_distribution<T> dist(low, high);
        return dist(gen);
    }
}
}
// int main() {
//     // Pick an integer between 1 and 100 (inclusive)
//     int randomInt = getRandomNumber(1, 100);
//     std::cout << "Random Int: " << randomInt << "\n";

//     // Pick a float between 1.5 and 9.5 (inclusive)
//     double randomDouble = getRandomNumber(1.5, 9.5);
//     std::cout << "Random Double: " << randomDouble << "\n";

//     return 0;
// }
