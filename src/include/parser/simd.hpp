#pragma once
#include <generator>
#include <iostream>
#include <istream>
#include <simdjson.h>
#include <string>

// Minimal line source for GenericParser coroutine API
struct LineGenerator {
  std::istream *in_;
  LineGenerator(std::istream &in) : in_(&in) {}
  std::generator<std::string> operator()() {
    for (std::string line; std::getline(*in_, line);)
      co_yield line;
  }
};

// For sync parsing, provide a struct with next_line() ->
// std::optional<std::string>
template <typename... Args>

void domparse(Args &&...args) {

  // load from `twitter.json` file:
  simdjson::dom::parser parser;
  simdjson::dom::element tweets = parser.load(std::forward<Args>(args)...);
  std::cout << uint64_t(tweets["search_metadata"]["count"]) << " results."
            << std::endl;

  // Parse and iterate through an array of objects
  auto abstract_json = R"( [
        {  "12345" : {"a":12.34, "b":56.78, "c": 9998877}   },
        {  "12545" : {"a":11.44, "b":12.78, "c": 11111111}  }
        ] )"_padded;

  for (simdjson::dom::object obj : parser.parse(abstract_json)) {
    for (const auto key_value : obj) {
      std::cout << "key: " << key_value.key << " : ";
      simdjson::dom::object innerobj = key_value.value;
      std::cout << "a: " << double(innerobj["a"]) << ", ";
      std::cout << "b: " << double(innerobj["b"]) << ", ";
      std::cout << "c: " << int64_t(innerobj["c"]) << std::endl;
    }
  }
}

// For sync parsing, provide a struct with next_line() ->
// std::optional<std::string>
template <typename... Args> void ondemandparse(Args &&...args) {
  simdjson::ondemand::parser parser;
  simdjson::padded_string json = simdjson::padded_string::load(std::forward<Args>(args)...);
  simdjson::ondemand::document tweets = parser.iterate(json);
  std::cout << uint64_t(tweets["search_metadata"]["count"]) << " results."
            << std::endl;
};

template <typename... Args> void promptparse(Args &&...args) {

  // load from `twitter.json` file:
  simdjson::dom::parser parser;
  simdjson::dom::element tweets = parser.load(std::forward<Args>(args)...);
  std::string arg1, arg2;
  std::cin >> arg1 >> arg2;
  std::cout << tweets[arg1][arg2] << " results." << std::endl;
};
/* SIMD USAGE


#include "simdjson.h"

int main(void) {
  // load from `twitter.json` file:
  simdjson::dom::parser parser;
  simdjson::dom::element tweets = parser.load("twitter.json");
  std::cout << tweets["search_metadata"]["count"] << " results." << std::endl;

  // Parse and iterate through an array of objects
  auto abstract_json = R"( [
                                { "12345" : {"a":12.34, "b":56.78, "c": 9998877}
}, { "12545" : {"a":11.44, "b":12.78, "c": 11111111}  } ] )"_padded;

  for (simdjson::dom::object obj : parser.parse(abstract_json)) {
    for (const auto key_value : obj) {
      std::cout << "key: " << key_value.key << " : ";
      simdjson::dom::object innerobj = key_value.value;
      std::cout << "a: " << double(innerobj["a"]) << ", ";
      std::cout << "b: " << double(innerobj["b"]) << ", ";
      std::cout << "c: " << int64_t(innerobj["c"]) << std::endl;
    }
  }
  //  R"( ... )" is a C++ raw string literal.
  auto cars_json = R"( [
                                { "make": "Toyota", "model": "Camry",  "year":
2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] }, { "make": "Kia", "model":
"Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] }, { "make":
"Toyota", "model": "Tercel", "year": 1999, "tire_pressure":
[ 29.8, 30.0, 30.2, 30.5 ] } ] )"_padded; simdjson::dom::parser parser2;

  // Iterating through an array of objects
  for (simdjson::dom::object car : parser.parse(cars_json)) {
    // Accessing a field by name
    std::cout << "Make/Model: " << car["make"] << "/" << car["model"]
              << std::endl;

    // Casting a JSON element to an integer
    uint64_t year = car["year"];
    std::cout << "- This car is " << 2020 - year << "years old." << std::endl;

    // Iterating through an array of floats
    double total_tire_pressure = 0;
    for (double tire_pressure : car["tire_pressure"]) {
      total_tire_pressure += tire_pressure;
    }
    std::cout << "- Average tire pressure: " << (total_tire_pressure / 4)
              << std::endl;

    // Writing out all the information about the car
    for (auto field : car) {
      std::cout << "- " << field.key << ": " << field.value << std::endl;
    }
  }
  return 0;
}

//JSON to OBJECT

struct Car {
    std::string make;
    std::string model;
    uint64_t year;
};
 ondemand::parser parser;
    auto json = R"( [
        { "make": "Toyota", "model": "Camry", "year": 2022 },
        { "make": "Honda", "model": "Civic", "year": 2021 }
    ] )"_padded;

    std::vector<Car> cars;
    ondemand::document doc = parser.iterate(json);

    // Iterating through an array of objects
    for (ondemand::object obj : doc) {
        Car car;
        // Accessing fields and converting them to C++ types
        car.make = std::string(std::string_view(obj["make"]));
        car.model = std::string(std::string_view(obj["model"]));
        car.year = obj["year"];
        cars.push_back(car);
    }


    //BUILDING JSON STRINGS
#include "simdjson.h"
#include <format> // Requires C++20 for std::format

simdjson::padded_string generate_sensor_data(int count) {
    simdjson::padded_string_builder builder;
    builder.append("["); // Start root array

    for (int i = 0; i < count; ++i) {
        if (i > 0) builder.append(",");
        
        // Constructing an object using raw string snippets
        builder.append(std::format(
            R"({{"id": {}, "temp": {:.2f}, "active": {}}})", 
            i, 20.5 + i, (i % 2 == 0 ? "true" : "false")
        ));
    }

    builder.append("]"); // End root array
    return builder.convert(); // Returns a padded_string ready for parsing
}

//ACCESSING OPTIONAL AND NESTED FIELDS
void process_user(ondemand::object user) {
    // Handling optional fields
    auto bio = user["bio"];
    if (bio.error() == error_code::SUCCESS) {
        std::cout << "Bio: " << std::string_view(bio) << std::endl;
    } else {
        std::cout << "Bio not provided." << std::endl;
    }

    // Accessing nested arrays
    ondemand::array tags;
    if (!user["tags"].get_array().get(tags)) {
        for (auto tag : tags) {
            std::cout << "Tag: " << std::string_view(tag) << std::endl;
        }
    }
}

    */