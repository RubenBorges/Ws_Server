#pragma once // Prevents recursive header inclusion bugs

#include <algorithm>
#include <boost/random/random_device.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include "../dir_crawler.hpp"
#include "../FBP_Tree.hpp"
#include "../spanningtree.hpp"
#include <flat_map>
#include <format>
#include <functional>
#include <iostream>
#include <jwt-cpp/jwt.h>
#include <print>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

enum class OP:int{NOP = 0,TX=1,RX=2};
std::vector<std::string> FileStrings;

