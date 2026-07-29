#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <iterator>

namespace fbptree
{
using Key   = std::int64_t;
using Value = std::string;

class FBPlusTree
{
private:
    struct Node;
    struct LeafNode;

public:
    FBPlusTree() = default;
    FBPlusTree(const FBPlusTree& other);
    FBPlusTree(FBPlusTree&&) noexcept = default;
    FBPlusTree& operator=(const FBPlusTree& other);
    FBPlusTree& operator=(FBPlusTree&&) noexcept = default;
    ~FBPlusTree() = default;

    static constexpr std::size_t kMinDegree = 32;

    bool empty() const noexcept;
    std::size_t size() const noexcept;

    void clear() noexcept;

    void insert(const Key& key, const Value& value);
    std::optional<Value> find(const Key& key) const;

    class iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = std::pair<const Key&, const Value&>;
        using difference_type   = std::ptrdiff_t;
        using reference         = value_type;

        iterator() = default;

        reference operator*() const;
        iterator& operator++();
        iterator operator++(int);

        friend bool operator==(const iterator& a, const iterator& b)
        {
            return a.leaf_ == b.leaf_ && a.index_ == b.index_;
        }
        friend bool operator!=(const iterator& a, const iterator& b)
        {
            return !(a == b);
        }

    private:
        const LeafNode* leaf_ = nullptr;
        std::size_t index_ = 0;

        iterator(const LeafNode* leaf, std::size_t index)
            : leaf_(leaf), index_(index) {}

        friend class FBPlusTree;
    };

    iterator begin() const;
    iterator end() const noexcept;

private:
    struct Node
    {
        bool is_leaf = false;
        std::vector<Key> keys;
        std::vector<std::unique_ptr<Node>> children;
        virtual ~Node() = default;
    };

    struct LeafNode : Node
    {
        std::vector<Value> values;
        LeafNode* next = nullptr;

        LeafNode() { this->is_leaf = true; }
    };

    std::unique_ptr<Node> root_;
    std::size_t size_ = 0;

    static std::unique_ptr<Node> clone_node(const Node* src,
                                            LeafNode*& left,
                                            LeafNode*& right);

    static std::unique_ptr<Node> clone_internal(const Node* src,
                                                LeafNode*& left,
                                                LeafNode*& right);

    LeafNode* find_leaf(const Key& key);
    const LeafNode* find_leaf(const Key& key) const;

    void insert_non_full(Node* node, const Key& key, const Value& value);
    void split_child(Node* parent, std::size_t index);

    LeafNode* leftmost_leaf();
    const LeafNode* leftmost_leaf() const;
};

} // namespace fbptree
