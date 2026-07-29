#include <FBP_Tree.hpp>
#include <algorithm>
#include <cassert>

namespace fbptree
{
namespace {
std::size_t max_keys() { return 2 * FBPlusTree::kMinDegree - 1; }

// lower_bound index — used for leaf key search
template<class Vec, class T>
std::size_t lb(const Vec& v, const T& x)
{
    return std::distance(v.begin(),
                         std::lower_bound(v.begin(), v.end(), x));
}

// upper_bound index — used for internal node child selection.
// In a B+ tree, child[i] holds keys in [keys[i-1], keys[i]), so to locate
// the correct child for `x` we need the first key that is strictly > x.
template<class Vec, class T>
std::size_t ub(const Vec& v, const T& x)
{
    return std::distance(v.begin(),
                         std::upper_bound(v.begin(), v.end(), x));
}
}

// ---------- iterator ----------

FBPlusTree::iterator::reference FBPlusTree::iterator::operator*() const
{
    return { leaf_->keys[index_], leaf_->values[index_] };
}

FBPlusTree::iterator& FBPlusTree::iterator::operator++()
{
    ++index_;
    if (!leaf_ || index_ >= leaf_->keys.size())
    {
        if (leaf_) leaf_ = leaf_->next;
        index_ = 0;
    }
    return *this;
}

FBPlusTree::iterator FBPlusTree::iterator::operator++(int)
{
    iterator tmp = *this;
    ++(*this);
    return tmp;
}

// ---------- cloning ----------

std::unique_ptr<FBPlusTree::Node>
FBPlusTree::clone_internal(const Node* src,
                           LeafNode*& left,
                           LeafNode*& right)
{
    if (!src) return nullptr;

    if (src->is_leaf)
    {
        auto s = static_cast<const LeafNode*>(src);
        auto d = std::make_unique<LeafNode>();

        d->keys   = s->keys;
        d->values = s->values;
        d->next   = nullptr;

        left = right = d.get();
        return d;
    }

    auto d = std::make_unique<Node>();
    d->is_leaf = false;
    d->keys = src->keys;

    LeafNode* first = nullptr;
    LeafNode* last  = nullptr;

    for (auto& c : src->children)
    {
        LeafNode* cl = nullptr;
        LeafNode* cr = nullptr;

        auto child = clone_internal(c.get(), cl, cr);

        if (!first) first = cl;
        if (last && cl) last->next = cl;
        if (cr) last = cr;

        d->children.push_back(std::move(child));
    }

    left = first;
    right = last;
    return d;
}

std::unique_ptr<FBPlusTree::Node>
FBPlusTree::clone_node(const Node* src,
                       LeafNode*& left,
                       LeafNode*& right)
{
    left = right = nullptr;
    return clone_internal(src, left, right);
}

// ---------- constructors / assignment ----------

FBPlusTree::FBPlusTree(const FBPlusTree& other)
    : size_(other.size_)
{
    LeafNode* l = nullptr;
    LeafNode* r = nullptr;
    root_ = clone_node(other.root_.get(), l, r);
}

FBPlusTree& FBPlusTree::operator=(const FBPlusTree& other)
{
    if (this == &other) return *this;

    FBPlusTree tmp(other);
    std::swap(root_, tmp.root_);
    std::swap(size_, tmp.size_);
    return *this;
}

// ---------- basic ops ----------

bool FBPlusTree::empty() const noexcept { return size_ == 0; }
std::size_t FBPlusTree::size() const noexcept { return size_; }

void FBPlusTree::clear() noexcept
{
    root_.reset();
    size_ = 0;
}

// ---------- search helpers ----------

const FBPlusTree::LeafNode*
FBPlusTree::find_leaf(const Key& key) const
{
    const Node* n = root_.get();
    if (!n) return nullptr;

    while (!n->is_leaf)
    {
        // upper_bound: child[i] contains keys in [keys[i-1], keys[i]),
        // so we follow child[first index where keys[i] > key].
        auto idx = ub(n->keys, key);
        n = n->children[idx].get();
    }
    return static_cast<const LeafNode*>(n);
}

FBPlusTree::LeafNode*
FBPlusTree::find_leaf(const Key& key)
{
    return const_cast<LeafNode*>(std::as_const(*this).find_leaf(key));
}

FBPlusTree::LeafNode* FBPlusTree::leftmost_leaf()
{
    Node* n = root_.get();
    if (!n) return nullptr;
    while (!n->is_leaf) n = n->children.front().get();
    return static_cast<LeafNode*>(n);
}

const FBPlusTree::LeafNode* FBPlusTree::leftmost_leaf() const
{
    return const_cast<FBPlusTree*>(this)->leftmost_leaf();
}

std::optional<Value> FBPlusTree::find(const Key& key) const
{
    auto leaf = find_leaf(key);
    if (!leaf) return std::nullopt;

    auto idx = lb(leaf->keys, key);
    if (idx < leaf->keys.size() && leaf->keys[idx] == key)
        return leaf->values[idx];

    return std::nullopt;
}

// ---------- splitting ----------

void FBPlusTree::split_child(Node* parent, std::size_t i)
{
    auto& child_ptr = parent->children[i];
    Node* child = child_ptr.get();

    if (child->is_leaf)
    {
        auto old = static_cast<LeafNode*>(child);
        auto neu = std::make_unique<LeafNode>();

        std::size_t mid = old->keys.size() / 2;

        neu->keys.assign(old->keys.begin() + mid, old->keys.end());
        neu->values.assign(old->values.begin() + mid, old->values.end());

        old->keys.resize(mid);
        old->values.resize(mid);

        neu->next = old->next;
        old->next = neu.get();

        Key promote = neu->keys.front();

        parent->keys.insert(parent->keys.begin() + i, promote);
        parent->children.insert(parent->children.begin() + i + 1,
                                std::move(neu));
    }
    else
    {
        auto neu = std::make_unique<Node>();
        neu->is_leaf = false;

        std::size_t mid = child->keys.size() / 2;

        Key promote = child->keys[mid];

        // Standard B-tree internal split:
        // left keeps [0, mid), right gets (mid, end)
        neu->keys.assign(child->keys.begin() + mid + 1, child->keys.end());
        child->keys.resize(mid);

        neu->children.assign(
            std::make_move_iterator(child->children.begin() + mid + 1),
            std::make_move_iterator(child->children.end()));

        child->children.resize(mid + 1);

        parent->keys.insert(parent->keys.begin() + i, promote);
        parent->children.insert(parent->children.begin() + i + 1,
                                std::move(neu));
    }
}

// ---------- insert ----------

void FBPlusTree::insert_non_full(Node* node, const Key& key, const Value& value)
{
    if (node->is_leaf)
    {
        auto leaf = static_cast<LeafNode*>(node);
        auto idx = lb(leaf->keys, key);

        if (idx < leaf->keys.size() && leaf->keys[idx] == key)
        {
            leaf->values[idx] = value;
            return;
        }

        leaf->keys.insert(leaf->keys.begin() + idx, key);
        leaf->values.insert(leaf->values.begin() + idx, value);
        ++size_;
        return;
    }

    // Use upper_bound so a key equal to a separator routes to the right
    // child (where that key lives in the leaf level).
    auto idx = ub(node->keys, key);
    Node* child = node->children[idx].get();

    if (child->keys.size() >= max_keys())
    {
        split_child(node, idx);
        // After the split, node->keys[idx] is the promoted separator.
        // Left child holds keys < separator; right child holds keys >= separator.
        // Use >= so an exact match on the separator routes right.
        if (key >= node->keys[idx])
            child = node->children[idx + 1].get();
    }

    insert_non_full(child, key, value);
}

void FBPlusTree::insert(const Key& key, const Value& value)
{
    if (!root_)
    {
        auto leaf = std::make_unique<LeafNode>();
        leaf->keys.push_back(key);
        leaf->values.push_back(value);
        root_ = std::move(leaf);
        size_ = 1;
        return;
    }

    if (root_->keys.size() >= max_keys())
    {
        auto new_root = std::make_unique<Node>();
        new_root->is_leaf = false;
        new_root->children.push_back(std::move(root_));
        root_ = std::move(new_root);

        split_child(root_.get(), 0);
    }

    insert_non_full(root_.get(), key, value);
}

// ---------- iteration ----------

FBPlusTree::iterator FBPlusTree::begin() const
{
    auto leaf = leftmost_leaf();
    if (!leaf || leaf->keys.empty())
        return end();
    return iterator(leaf, 0);
}

FBPlusTree::iterator FBPlusTree::end() const noexcept
{
    return iterator();
}

} // namespace fbptree
