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
#include <format>
#include <functional>
#include <iostream>
#include <jwt-cpp/jwt.h>
#include <print>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>


namespace data {
enum class OP:int{NOP = 0,TX=1,RX=2, NEW =3, DEL = 4};
enum class Result:int{SUCCESS = 0,FAILURE=1,PENDING=2};
    struct dataRequest{
        std::vector<std::string>& name;
        std::filesystem::path rootPath; 
        OP op{0};
        Result result{2};
    };
}

int main (){
    std::vector<std::string> files;
    std::vector<std::string> FileStrings;
    files.reserve(50); FileStrings.reserve(50);
    std::filesystem::path filetarget;
    static data::dataRequest req{files,filetarget,data::OP::NEW};
    std::cout<<"Enter filenames Directory: [filename] [filename] [filename]... \n>";
    for(std::cin>>filetarget;filetarget!=""||filetarget!="c";){
        files.emplace_back(filetarget);
        std::cout<<std::endl;
    }

}