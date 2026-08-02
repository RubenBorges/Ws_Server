#pragma once

#include <RequestRouter/datahandler.hpp>
#include <RequestRouter/loginhandler.hpp>
#include <algorithm> 
#include <cerrno>    
#include <concepts>
#include <filesystem>
#include <cstring> 
#include <fcntl.h> 
#include <iostream>
#include <random>
#include <sys/stat.h>
#include <sys/syscall.h> 
#include <fstream>
#include <unistd.h>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <string>
#include <type_traits>

namespace bpy {

namespace utility {

// FIXED: Added std::move because io_context cannot be copied
inline boost::asio::io_context make_io_context(int thread_count = 1) {
    return boost::asio::io_context(thread_count); 
}

inline std::shared_ptr<boost::asio::io_context> make_shared_io_context(int thread_count = 1) {
    return std::make_shared<boost::asio::io_context>(thread_count);
}

inline bool isTargetDirectory(const std::string &path) {
    struct stat pathStat;
    if (::stat(path.c_str(), &pathStat) != 0) {
        std::cerr << "Metadata read failed for [" << path
                  << "] | Error: " << std::strerror(errno) << "\n";
        return false;
    }
    return S_ISDIR(pathStat.st_mode);
}

inline bool removeTargetViaSyscall(const std::string &pathTarget) {
    int flags = isTargetDirectory(pathTarget) ? AT_REMOVEDIR : 0;

    auto result = syscall(SYS_unlinkat, AT_FDCWD, pathTarget.c_str(), flags);

    if (result == 0)
        return true;
    return false; 
}

inline bool createEmptyFile(const std::filesystem::path& filePath) {
    if (filePath.has_parent_path()) {     
        std::filesystem::create_directories(filePath.parent_path());
    }
    std::ofstream file(filePath);    
    if (file.is_open()) {
        std::cout << "File created successfully: " << filePath << "\n";
    } else {
        std::cerr << "Failed to create file: " << filePath << "\n";
        return false;
    }
    return true;
}

// FIXED: Wrapped int_distribution with conditional compilation to support short ints/chars
template <typename T>
  requires std::integral<T> || std::floating_point<T>
T getRandomNumber(T x, T y) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    T low = std::min(x, y);
    T high = std::max(x, y);

    if constexpr (std::floating_point<T>) {
        std::uniform_real_distribution<T> dist(low, high);
        return dist(gen);
    } else {
        // Boost/std::uniform_int_distribution doesn't allow char, uint8_t, etc.
        // We type-promote types smaller than int safely at compile time.
        using DistributionType = std::conditional_t<
            sizeof(T) < sizeof(int), 
            int, 
            T
        >;
        std::uniform_int_distribution<DistributionType> dist(static_cast<DistributionType>(low), static_cast<DistributionType>(high));
        return static_cast<T>(dist(gen));
    }
}

} // namespace utility
} // namespace bpy
