#include "DirectoryCrawler.hpp"

#include <cstdlib>
#include <cstddef>
#include <algorithm>
#include <filesystem>
#include <iterator>
#include <ranges>

namespace {

std::filesystem::path validatedDirectoryPath(const std::string& dirPathStr) {
    const std::filesystem::path candidate(dirPathStr);

    if (!std::filesystem::exists(candidate)) {
        throw std::filesystem::filesystem_error(
            "Directory does not exist!",
            candidate,
            std::make_error_code(std::errc::no_such_file_or_directory));
    }

    if (!std::filesystem::is_directory(candidate)) {
        throw std::filesystem::filesystem_error(
            "Path is not a directory!",
            candidate,
            std::make_error_code(std::errc::not_a_directory));
    }

    return std::filesystem::absolute(candidate).lexically_normal();
}

bool containsPath(const std::vector<std::filesystem::path>& paths,
                  const std::filesystem::path& candidate) {
    return std::ranges::find(paths, candidate) != paths.end();
}

} // namespace


DirectoryCrawler::DirectoryCrawler(const std::string& dirPathStr)
    : sourceDirectory(validatedDirectoryPath(dirPathStr)) {
    sourceDirectoryList.emplace_back(sourceDirectory);
}


//Returns std Directory_entry objects
int DirectoryCrawler::crawl(std::vector<std::filesystem::directory_entry>& OUT_ListOfEntries) {
    try {
        auto iter = std::filesystem::directory_iterator(sourceDirectory);
        std::ranges::copy(iter, std::back_inserter(OUT_ListOfEntries));
        lastError = "No Error Set";
        return EXIT_SUCCESS;
    } catch (const std::filesystem::filesystem_error& e) {
        lastError = e.what();
        return EXIT_FAILURE;
    }
}

int DirectoryCrawler::crawl(std::vector<std::vector<std::filesystem::directory_entry>>& OUT_ListOfEntries) {
    try {
        if (OUT_ListOfEntries.size() < sourceDirectoryList.size()) {
            OUT_ListOfEntries.resize(sourceDirectoryList.size());
        }

        for (std::size_t i = 0; i < sourceDirectoryList.size(); ++i) {
            auto iter = std::filesystem::directory_iterator(sourceDirectoryList[i]);
            std::ranges::copy(iter, std::back_inserter(OUT_ListOfEntries[i]));
        }

        lastError = "No Error Set";
        return EXIT_SUCCESS;
    } catch (const std::filesystem::filesystem_error& e) {
        lastError = e.what();
        return EXIT_FAILURE;
    }
}

// 1. Returned the filenames (e.g., "devilboy.obj")
int DirectoryCrawler::crawl(std::vector<std::string>& OUT_ListOfEntries) {
    try {
        auto view = std::filesystem::directory_iterator(sourceDirectory) 

                    | std::views::transform([](const auto& e) { 
                        return e.path().filename().string(); 
                    });
        
        std::ranges::copy(view, std::back_inserter(OUT_ListOfEntries));
        lastError = "No Error Set";
        return EXIT_SUCCESS;
    } catch (const std::filesystem::filesystem_error& e) {
        lastError = e.what();
        return EXIT_FAILURE;
    }
}

// 2. Returns recursive paths (Full relative paths)
int DirectoryCrawler::crawlRecursively(std::vector<std::string>& OUT_ListOfEntries) {
    try {
        auto iter = std::filesystem::recursive_directory_iterator(
            sourceDirectory, 
            std::filesystem::directory_options::skip_permission_denied
        );

        auto view = iter | std::views::transform([](const auto& e) { 
                        return e.path().string(); 
                    });

        std::ranges::copy(view, std::back_inserter(OUT_ListOfEntries));
        lastError = "No Error Set";
        return EXIT_SUCCESS;
    } catch (const std::filesystem::filesystem_error& e) {
        lastError = e.what();
        return EXIT_FAILURE;
    }
}
// 3. Returns absolute paths
int DirectoryCrawler::crawlAbsolute(std::vector<std::string>& OUT_ListOfEntries) {
    try {
        auto view = std::filesystem::directory_iterator(sourceDirectory)
                    | std::views::transform([](const auto& e) {
                        // Ensure we return the absolute path
                        return std::filesystem::absolute(e.path()).string();
                    });

        std::ranges::copy(view, std::back_inserter(OUT_ListOfEntries));
        lastError = "No Error Set";
        return EXIT_SUCCESS;
    } catch (const std::filesystem::filesystem_error& e) {
        lastError = e.what();
        return EXIT_FAILURE;
    }
}

int DirectoryCrawler::changeDirectory(const std::string& dirPathStr) {
    try {
        sourceDirectory = validatedDirectoryPath(dirPathStr);
        if (!containsPath(sourceDirectoryList, sourceDirectory)) {
            sourceDirectoryList.emplace_back(sourceDirectory);
        }

        lastError = "No Error Set";
        return EXIT_SUCCESS;
    } catch (const std::filesystem::filesystem_error& e) {
        lastError = e.what();
        return EXIT_FAILURE;
    }
}

int DirectoryCrawler::newDirectory(const std::string& dirPathStr) {
    try {
        const std::filesystem::path newPath = validatedDirectoryPath(dirPathStr);
        if (!containsPath(sourceDirectoryList, newPath)) {
            sourceDirectoryList.emplace_back(newPath);
        }

        sourceDirectory = newPath;
        lastError = "No Error Set";
        return EXIT_SUCCESS;
    } catch (const std::filesystem::filesystem_error& e) {
        lastError = e.what();
        return EXIT_FAILURE;
    }
}