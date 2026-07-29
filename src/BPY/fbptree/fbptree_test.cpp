#include <fbptree/fbptree.hpp>

#include <cassert>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace fbptree;
static void test_increasing_inserts_debug()
{
    FBPlusTree t;
    const int N = 5000;

    for (int i = 0; i < N; ++i)
        t.insert(i, "v" + std::to_string(i));

    if (t.size() != static_cast<std::size_t>(N)) {
        std::cout << "[WARN] size mismatch after inserts: expected " << N << " got " << t.size() << "\n";
    }

    int missing = -1;
    for (int i = 0; i < N; ++i) {
        auto opt = t.find(i);
        if (!opt.has_value()) {
            missing = i;
            std::cout << "[ERROR] missing key: " << i << "\n";
            break;
        }
        if (opt.value() != ("v" + std::to_string(i))) {
            std::cout << "[ERROR] value mismatch for key " << i << ": got '" << opt.value()
            << "' expected '" << ("v" + std::to_string(i)) << "'\n";
            missing = i;
            break;
        }
    }

    if (missing == -1) {
        std::cout << "[OK] increasing inserts (all " << N << " keys present)\n";
        return;
    }

    // If missing, print nearby keys via iterator to inspect leaf contents.
    std::cout << "Dumping nearby keys around missing key " << missing << ":\n";

    // Find the leaf that would contain 'missing' by scanning iterator
    int printed = 0;
    for (auto it = t.begin(); it != t.end(); ++it) {
        auto [k, v] = *it;
        if (k >= missing - 10 && k <= missing + 10) {
            std::cout << k << " -> " << v << "\n";
            ++printed;
        }
        if (k > missing + 10) break;
    }

    if (printed == 0) {
        std::cout << "[INFO] no nearby keys printed (leaf chain may be broken)\n";
    }

    // Also print first 20 keys and last 20 keys to inspect global shape
    std::cout << "First 20 keys:\n";
    int cnt = 0;
    for (auto it = t.begin(); it != t.end() && cnt < 20; ++it, ++cnt) {
        auto [k, v] = *it;
        std::cout << k << " ";
    }
    std::cout << "\nLast 20 keys:\n";
    // To get last 20, iterate and keep a ring buffer
    std::vector<int> ring;
    for (auto it = t.begin(); it != t.end(); ++it) {
        auto [k, v] = *it;
        ring.push_back(static_cast<int>(k));
        if (ring.size() > 20) ring.erase(ring.begin());
    }
    for (int x : ring) std::cout << x << " ";
    std::cout << "\n";
}

static void test_empty_tree()
{
    FBPlusTree t;
    assert(t.empty());
    assert(t.size() == 0);
    assert(!t.find(123).has_value());
    std::cout << "[OK] empty tree\n";
}

static void test_single_insert()
{
    FBPlusTree t;
    t.insert(10, "ten");

    assert(!t.empty());
    assert(t.size() == 1);

    auto v = t.find(10);
    assert(v.has_value());
    assert(v.value() == "ten");

    std::cout << "[OK] single insert\n";
}

static void test_multiple_inserts_and_search()
{
    FBPlusTree t;

    t.insert(5, "five");
    t.insert(1, "one");
    t.insert(9, "nine");
    t.insert(3, "three");
    t.insert(7, "seven");

    assert(t.size() == 5);

    assert(t.find(1).value() == "one");
    assert(t.find(3).value() == "three");
    assert(t.find(5).value() == "five");
    assert(t.find(7).value() == "seven");
    assert(t.find(9).value() == "nine");

    assert(!t.find(2).has_value());
    assert(!t.find(8).has_value());

    std::cout << "[OK] multiple inserts + search\n";
}

static void test_overwrite_existing_key()
{
    FBPlusTree t;

    t.insert(42, "first");
    t.insert(42, "second");

    assert(t.size() == 1);
    assert(t.find(42).value() == "second");

    std::cout << "[OK] overwrite existing key\n";
}

static void test_increasing_inserts()
{
    FBPlusTree t;

    for (int i = 0; i < 5000; ++i)
        t.insert(i, "v" + std::to_string(i));

    assert(t.size() == 5000);

    for (int i = 0; i < 5000; ++i)
        assert(t.find(i).value() == "v" + std::to_string(i));

    std::cout << "[OK] increasing inserts (5000 keys)\n";
}

static void test_decreasing_inserts()
{
    FBPlusTree t;

    for (int i = 5000; i >= 0; --i)
        t.insert(i, "v" + std::to_string(i));

    assert(t.size() == 5001);

    for (int i = 0; i <= 5000; ++i)
        assert(t.find(i).value() == "v" + std::to_string(i));

    std::cout << "[OK] decreasing inserts (5001 keys)\n";
}

static void test_random_inserts()
{
    FBPlusTree t;
    std::set<int> inserted;

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(0, 20000);

    for (int i = 0; i < 10000; ++i)
    {
        int k = dist(rng);
        t.insert(k, "v" + std::to_string(k));
        inserted.insert(k);
    }

    assert(t.size() == inserted.size());

    for (int k : inserted)
        assert(t.find(k).value() == "v" + std::to_string(k));

    std::cout << "[OK] random inserts (10000 ops)\n";
}

static void test_iterator_sorted_order()
{
    FBPlusTree t;

    for (int i = 100; i >= 0; --i)
        t.insert(i, "v" + std::to_string(i));

    int expected = 0;
    for (auto it = t.begin(); it != t.end(); ++it)
    {
        auto [k, v] = *it;
        assert(k == expected);
        assert(v == "v" + std::to_string(expected));
        expected++;
    }

    assert(expected == 101);

    std::cout << "[OK] iterator sorted order\n";
}

static void test_iterator_exhaustion()
{
    FBPlusTree t;

    for (int i = 0; i < 50; ++i)
        t.insert(i, "v" + std::to_string(i));

    auto it = t.begin();
    int count = 0;

    while (it != t.end())
    {
        ++it;
        ++count;
    }

    assert(count == 50);

    std::cout << "[OK] iterator exhaustion\n";
}

static void test_leaf_chain_integrity()
{
    FBPlusTree t;

    for (int i = 0; i < 2000; ++i)
        t.insert(i, "v" + std::to_string(i));

    // Walk via iterator
    int count = 0;
    int last = -1;

    for (auto it = t.begin(); it != t.end(); ++it)
    {
        auto [k, v] = *it;
        assert(k == last + 1);
        last = k;
        count++;
    }

    assert(count == 2000);

    std::cout << "[OK] leaf chain integrity\n";
}

static void test_copy_constructor()
{
    FBPlusTree t;

    for (int i = 0; i < 1000; ++i)
        t.insert(i, "v" + std::to_string(i));

    FBPlusTree copy = t;

    assert(copy.size() == t.size());

    for (int i = 0; i < 1000; ++i)
        assert(copy.find(i).value() == "v" + std::to_string(i));

    std::cout << "[OK] copy constructor\n";
}

static void test_assignment_operator()
{
    FBPlusTree t;

    for (int i = 0; i < 500; ++i)
        t.insert(i, "v" + std::to_string(i));

    FBPlusTree t2;
    t2 = t;

    assert(t2.size() == t.size());

    for (int i = 0; i < 500; ++i)
        assert(t2.find(i).value() == "v" + std::to_string(i));

    std::cout << "[OK] assignment operator\n";
}

static void test_clear()
{
    FBPlusTree t;

    for (int i = 0; i < 1000; ++i)
        t.insert(i, "v" + std::to_string(i));

    t.clear();

    assert(t.empty());
    assert(t.size() == 0);
    assert(!t.find(10).has_value());

    std::cout << "[OK] clear()\n";
}

static void test_mixed_operations()
{
    FBPlusTree t;

    for (int i = 0; i < 2000; ++i)
        t.insert(i, "v" + std::to_string(i));

    for (int i = 0; i < 2000; i += 2)
        t.insert(i, "even" + std::to_string(i));

    for (int i = 0; i < 2000; ++i)
    {
        if (i % 2 == 0)
            assert(t.find(i).value() == "even" + std::to_string(i));
        else
            assert(t.find(i).value() == "v" + std::to_string(i));
    }

    std::cout << "[OK] mixed operations\n";
}

int main()
{
    test_empty_tree();
    test_single_insert();
    test_multiple_inserts_and_search();
    test_overwrite_existing_key();
    test_increasing_inserts();
    test_decreasing_inserts();
    test_random_inserts();
    test_iterator_sorted_order();
    test_iterator_exhaustion();
    test_leaf_chain_integrity();
    test_copy_constructor();
    test_assignment_operator();
    test_clear();
    test_mixed_operations();

    std::cout << "\nALL TESTS PASSED\n";
    return 0;
}
