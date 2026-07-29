

#pragma once
#include <iostream>
#include <vector>
#include <stdexcept>

class Graph {
private:
    int numVertices;
    std::vector<std::vector<int>> adj;

public:
    explicit Graph(int n)
    : numVertices(n), adj(n) {}

    void addEdge(int u, int v) {
        if (u < 0 || v < 0 || u >= numVertices || v >= numVertices)
            throw std::out_of_range("Invalid vertex index");

        adj[u].push_back(v);
        adj[v].push_back(u);
    }

    const std::vector<int>& neighbors(int u) const {
        return adj.at(u);
    }
    void printGraph() const {
            for (int i = 0; i < numVertices; ++i) {
            std::cout << i << ": ";
                for (int neighbor : adj[i]) {
                std::cout << neighbor << " ";
            }
        std::cout << "\n";
        }
    }
    int size() const noexcept { return numVertices; }
};


// // #include <vector>
// // #include <iostream>
// // #include <algorithm>
// //
// // class Graph {
// // private:
// //     int numVertices;
// //     // adjacencyList[u] contains a list of all neighbors 'v'
// //     std::vector<std::vector<int>> adjList;
// //
// // public:
// //     explicit Graph(int vertices) : numVertices(vertices) {
// //         adjList.resize(vertices);
// //     }
// //
// //     // Add an undirected edge (typical for network spanning trees)
// //     void addEdge(int u, int v) {
// //         if (u >= 0 && u < numVertices && v >= 0 && v < numVertices) {
// //             adjList[u].push_back(v);
// //             adjList[v].push_back(u);
// //         }
// //     }
// //
// //     // Getter for the neighbors of a specific node
// //     const std::vector<int>& getNeighbors(int u) const {
// //         return adjList.at(u);
// //     }
// //
// //     int getNumVertices() const {
// //         return numVertices;
// //     }
// //
// //     void printGraph() const {
// //         for (int i = 0; i < numVertices; ++i) {
// //             std::cout << i << ": ";
// //             for (int neighbor : adjList[i]) {
// //                 std::cout << neighbor << " ";
// //             }
// //             std::cout << "\n";
// //         }
// //     }
// // };
