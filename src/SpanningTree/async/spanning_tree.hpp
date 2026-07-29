#pragma once
#include "Graph.hpp"
#include "Node.hpp"
#include <unordered_map>
#include <queue>
#include <memory>
#include <stdexcept>
#include <sstream>

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

    // ---------------- ASYNC EXECUTION ----------------

    void startAsync(const std::string& msg) {
        if (!root)
            throw std::runtime_error("Tree not built");

        // Initialize
        for (auto& [id, node] : nodes)
            node->resetState();

        root->enqueue({Message::Type::GO, 0, msg});

        runScheduler();
    }

    void runScheduler() {
        bool progress = true;

        while (progress) {
            progress = false;

            for (auto& [id, node] : nodes) {
                if (!node->inbox.empty()) {
                    processMessage(*node);
                    progress = true;
                }
            }
        }
    }
    Node* getRoot() const noexcept {
        return root;
    }

    Node* getNode(int id) const {
        auto it = nodes.find(id);
        return (it == nodes.end()) ? nullptr : it->second.get();
    }

    void processMessage(Node& node) {
        Message m = node.inbox.front();
        node.inbox.pop();

        if (m.type == Message::Type::GO) {
            std::cout << "Node " << node.id << " received: " << m.payload << "\n";

            if (node.isLeaf()) {
                if (node.isRoot())
                    printResult(node.localValue);
                else
                    node.parent->enqueue({Message::Type::BACK, node.localValue, ""});
                return;
            }

            for (Node* c : node.children)
                c->enqueue({Message::Type::GO, 0, m.payload});
        }
        else if (m.type == Message::Type::BACK) {
            node.accumulated += m.value;
            node.pendingChildren--;

            if (node.pendingChildren == 0) {
                int total = node.accumulated + node.localValue;

                if (node.isRoot())
                    printResult(total);
                else
                    node.parent->enqueue({Message::Type::BACK, total, ""});
            }
        }
    }

    // ---------------- VISUALIZATION ----------------

    std::string toASCII() const {
        std::ostringstream out;
        printASCII(root, "", true, out);
        return out.str();
    }

    std::string toDOT() const {
        std::ostringstream out;
        out << "digraph SpanningTree {\n";
        printDOT(root, out);
        out << "}\n";
        return out.str();
    }

    std::string toJSON() const {
        return jsonNode(root);
    }

private:
    void printResult(int total) const {
        std::cout << "\n>>> ASYNC CONVERGECAST COMPLETE <<<\n";
        std::cout << "Root " << root->id << " total sum = " << total << "\n\n";
    }

    void printASCII(Node* n, std::string prefix, bool last, std::ostringstream& out) const {
        if (!n) return;

        out << prefix << (last ? "└── " : "├── ") << n->id << "\n";

        for (size_t i = 0; i < n->children.size(); i++)
            printASCII(n->children[i], prefix + (last ? "    " : "│   "), i + 1 == n->children.size(), out);
    }

    void printDOT(Node* n, std::ostringstream& out) const {
        if (!n) return;
        for (Node* c : n->children) {
            out << "  " << n->id << " -> " << c->id << ";\n";
            printDOT(c, out);
        }
    }

    std::string jsonNode(Node* n) const {
        if (!n) return "null";

        std::ostringstream out;
        out << "{ \"id\": " << n->id << ", \"children\": [";

        for (size_t i = 0; i < n->children.size(); i++) {
            out << jsonNode(n->children[i]);
            if (i + 1 < n->children.size()) out << ",";
        }

        out << "] }";
        return out.str();
    }
};
