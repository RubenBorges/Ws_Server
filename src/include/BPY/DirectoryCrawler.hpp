#pragma once

#include <filesystem>
#include <string>
#include <vector>

//Class which is designed to take a:
// [INPUT]source directory and return 
// [OUTPUT]a list of discovered entries to a destination std::vector<std::string>&
class DirectoryCrawler {
    std::filesystem::path sourceDirectory;
    std::vector<std::filesystem::path> sourceDirectoryList;
    std::string lastError{"No Error Set"};

public:
    //Constructor: supply a directory path
    DirectoryCrawler(const std::string& rootPath);
    int crawl(std::vector<std::vector<std::filesystem::directory_entry>>& OUT_ListOfEntries);
    int crawl(std::vector<std::filesystem::directory_entry>& OUT_ListOfDirectoryEntries);
    //Non-recursive directory crawl.
    // [OUTPUT] pass in a string vector it will fill with directory entry addresses
    int crawl(std::vector<std::string>& OUT_ListOfEntriesPaths);
    int crawlAbsolute(std::vector<std::string>& OUT_ListOfEntriesAbsolutePaths);
    // Recursive directory crawl.
    // [OUTPUT] pass in a string vector it will fill with directory entry addresses
    int crawlRecursively(std::vector<std::string>& OUT_ListOfEntries);

    // Applying the same move optimization here
    int newDirectory(const std::string& dirPathStr);

    // Applying the same move optimization here
    int changeDirectory(const std::string& dirPathStr);

    const std::string& error() const noexcept { return lastError; }
};
