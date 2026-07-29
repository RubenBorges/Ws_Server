#pragma once
#include <vector>
#include <stdexcept>

class Graph {
private:
    int n;
    std::vector<std::vector<int>> adj;

public:
    explicit Graph(int n) : n(n), adj(n) {}

    void addEdge(int u, int v) {
        if (u < 0 || v < 0 || u >= n || v >= n)
            throw std::out_of_range("Invalid vertex index");
        adj[u].push_back(v);
        adj[v].push_back(u);
    }

    const std::vector<int>& neighbors(int u) const {
        return adj.at(u);
    }

    int size() const noexcept { return n; }
};
