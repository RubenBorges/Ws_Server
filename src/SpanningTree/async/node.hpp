#pragma once
#include <vector>
#include <queue>
#include <string>
#include <iostream>

struct Message {
    enum class Type { GO, BACK };
    Type type;
    int value;
    std::string payload;
};

struct Node {
    int id;
    int localValue;
    Node* parent;
    std::vector<Node*> children;

    int pendingChildren = 0;
    int accumulated = 0;

    std::queue<Message> inbox;

    explicit Node(int id, Node* parent)
    : id(id), localValue(id * 10), parent(parent) {}

    bool isRoot() const noexcept { return parent == nullptr; }
    bool isLeaf() const noexcept { return children.empty(); }

    void resetState() {
        accumulated = 0;
        pendingChildren = static_cast<int>(children.size());
    }

    void enqueue(Message m) {
        inbox.push(std::move(m));
    }
};
