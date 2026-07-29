

 #pragma once
 #include "graph.hpp"
 #include "node.hpp"
 #include <unordered_map>
 #include <queue>
 #include <memory>
 #include <stdexcept>

 class SpanningTree {
 private:
     std::unordered_map<int, std::unique_ptr<Node>> nodes;
     Node* root = nullptr;

 public:
     void build(const Graph& g, int rootId) {
         if (rootId < 0 || rootId >= g.size())
             throw std::out_of_range("Invalid root ID");

         nodes.clear();
         root = nullptr;

         std::vector<bool> visited(g.size(), false);
         std::queue<int> q;

         nodes[rootId] = std::make_unique<Node>(rootId, nullptr);
         root = nodes[rootId].get();
         visited[rootId] = true;
         q.push(rootId);

         while (!q.empty()) {
             int u = q.front();
             q.pop();

             for (int v : g.neighbors(u)) {
                 if (!visited[v]) {
                     visited[v] = true;

                     nodes[v] = std::make_unique<Node>(v, nodes[u].get());
                     nodes[u]->children.push_back(nodes[v].get());

                     q.push(v);
                 }
             }
         }
     }

     void start(const std::string& msg) {
         if (!root)
             throw std::runtime_error("Tree not built");

         root->start(msg);
     }

     Node* getRoot() const noexcept { return root; }
     Node* getNode(int id) const {
         auto it = nodes.find(id);
         return it == nodes.end() ? nullptr : it->second.get();
     }
 };
 /*
  #include <queue>
  #include <unordered_map>
  #include <memory>

  class SpanningTree {
  private:
      Node* rootNode{nullptr};
      // unique_ptr ensures no memory leaks for Nodes
      std::unordered_map<int, std::unique_ptr<Node>> nodes;

  public:
      void buildFromGraph(const Graph& g, int rootId) {
          std::queue<int> q;
          std::vector<bool> visited(g.getNumVertices(), false);

          // 1. Initialize Root
          nodes[rootId] = std::make_unique<Node>(rootId, nullptr);
          rootNode = nodes[rootId].get();

          visited[rootId] = true;
          q.push(rootId);

          // 2. Build Hierarchy via BFS
          while (!q.empty()) {
              int currentId = q.front();
              q.pop();

              for (int neighborId : g.getNeighbors(currentId)) {
                  if (!visited[neighborId]) {
                      visited[neighborId] = true;

                      // Create child with 'current' as parent
                      nodes[neighborId] = std::make_unique<Node>(neighborId, nodes[currentId].get());

                      // Link parent to child
                      nodes[currentId]->child.push_back(nodes[neighborId].get());

                      q.push(neighborId);
                  }
              }
          }
      }

      // Kick off the process
      void start(const std::string& msg) {
          if (rootNode) {
              rootNode->go(msg);
          } else {
              std::cerr << "Error: Tree not built yet!" << std::endl;
          }
      }
  };*/
