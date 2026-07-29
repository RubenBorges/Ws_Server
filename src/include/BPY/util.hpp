#pragma once

#include <algorithm> // For std::min and std::max
#include <cerrno>    // For tracking kernel error flags
#include <concepts>
#include <filesystem>
#include <cstring> // For std::strerror
#include <fcntl.h> // For AT_FDCWD and AT_REMOVEDIR flags [1]
#include <iostream>
#include <random>
#include <sys/stat.h>
#include <sys/syscall.h> // For SYS_unlinkat macro definitions [1]
#include <fstream>
#include <unistd.h> // For syscall() definitions

namespace bpy {
namespace utility {

bool isTargetDirectory(const std::string &path) {
  struct stat pathStat;
  // 1. Invoke the stat system call to pull the target metadata
  // Returns 0 on success, -1 on failure (e.g. path does not exist)
  if (::stat(path.c_str(), &pathStat) != 0) {
    std::cerr << "Metadata read failed for [" << path
              << "] | Error: " << std::strerror(errno) << "\n";
    return false;
  }
  // 2. Evaluate the st_mode bitmask using the built-in kernel macro
  return S_ISDIR(pathStat.st_mode);
}

bool removeTargetViaSyscall(const std::string &pathTarget) {
  // 1. Configure the removal flags based on the target type
  int flags = isTargetDirectory(pathTarget)
                  ? AT_REMOVEDIR
                  : 0; // If it's a directory, we must pass AT_REMOVEDIR,
                       // otherwise pass 0 for a file [1]

  // 2. Invoke the kernel directly
  auto result = syscall(SYS_unlinkat, AT_FDCWD, pathTarget.c_str(),
                        flags); // AT_FDCWD tells the kernel that 'path' is
                                // relative to the current working directory [1]

  // 3. Evaluate kernel return state (0 is success, -1 is failure)
  if (result == 0)
    return true;
  return false; // If it failed, the global 'errno' variable holds the exact
                // kernel failure reason
};
bool createEmptyFile(const std::filesystem::path& filePath) {

    if (filePath.has_parent_path()) {     // 1. Optional: Ensure the folder structure leading up to the file exists
        std::filesystem::create_directories(filePath.parent_path());
    }
    std::ofstream file(filePath);    // 2. Open the stream. Opening in output mode automatically creates the file if it's missing.
    if (file.is_open()) {std::cout << "File created successfully: " << filePath << "\n";
    } else{
        std::cerr << "Failed to create file: " << filePath << "\n";
        return false;
    }
    return true;
}

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
} // namespace utility
} // namespace bpy

// int main() {
//     // Pick an integer between 1 and 100 (inclusive)
//     int randomInt = getRandomNumber(1, 100);
//     std::cout << "Random Int: " << randomInt << "\n";

//     // Pick a float between 1.5 and 9.5 (inclusive)
//     double randomDouble = getRandomNumber(1.5, 9.5);
//     std::cout << "Random Double: " << randomDouble << "\n";

//     return 0;
// }
