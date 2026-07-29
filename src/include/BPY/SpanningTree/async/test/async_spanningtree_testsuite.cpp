#include "spanning_tree.hpp"
#include <iostream>
#include <cassert>
#include <random>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
std::cout << "\n[TEST] " << name << "\n";

#define CHECK(cond) \
do { \
        if (!(cond)) { \
            std::cerr << "  FAILED: " #cond " at line " << __LINE__ << "\n"; \
            ++tests_failed; \
    } else { \
            ++tests_passed; \
    } \
} while (0)


    // ------------------------------------------------------------
    // 1. Single Node Async Test
    // ------------------------------------------------------------
    void test_single_node_async() {
    TEST("Single Node Async");

    Graph g(1);
    SpanningTree st;
    st.build(g, 0);

    CHECK(st.getRoot()->id == 0);
    CHECK(st.getRoot()->isLeaf());

    st.startAsync("hello async");
}


// ------------------------------------------------------------
// 2. Simple Tree Async
// ------------------------------------------------------------
void test_simple_tree_async() {
    TEST("Simple Tree Async");

    Graph g(4);
    g.addEdge(0,1);
    g.addEdge(0,2);
    g.addEdge(1,3);

    SpanningTree st;
    st.build(g, 0);

    Node* r = st.getRoot();
    CHECK(r->id == 0);
    CHECK(r->children.size() == 2);

    st.startAsync("async message");
}


// ------------------------------------------------------------
// 3. Async Convergecast Sum
// ------------------------------------------------------------
void test_async_convergecast_sum() {
    TEST("Async Convergecast Sum");

    Graph g(5);
    g.addEdge(0,1);
    g.addEdge(0,2);
    g.addEdge(1,3);
    g.addEdge(1,4);

    SpanningTree st;
    st.build(g, 0);

    // Expected sum = sum(id * 10) = 0 + 10 + 20 + 30 + 40 = 100
    st.startAsync("compute sum async");
}


// ------------------------------------------------------------
// 4. Multiple Async Runs
// ------------------------------------------------------------
void test_multiple_async_runs() {
    TEST("Multiple Async Runs");

    Graph g(3);
    g.addEdge(0,1);
    g.addEdge(1,2);

    SpanningTree st;
    st.build(g, 0);

    st.startAsync("run 1");
    st.startAsync("run 2");
    st.startAsync("run 3");
}


// ------------------------------------------------------------
// 5. Disconnected Graph Async
// ------------------------------------------------------------
void test_disconnected_graph_async() {
    TEST("Disconnected Graph Async");

    Graph g(5);
    g.addEdge(0,1);
    g.addEdge(3,4);

    SpanningTree st;
    st.build(g, 0);

    CHECK(st.getNode(2) == nullptr);
    CHECK(st.getNode(3) == nullptr);
    CHECK(st.getNode(4) == nullptr);

    st.startAsync("async test");
}


// ------------------------------------------------------------
// 6. Random Graph Async
// ------------------------------------------------------------
void test_random_graph_async() {
    TEST("Random Graph Async");

    Graph g(50);
    for (int i = 0; i < 49; i++)
        g.addEdge(i, i+1);

    SpanningTree st;
    st.build(g, 0);

    st.startAsync("random async");
}


// ------------------------------------------------------------
// 7. Visualization Tests
// ------------------------------------------------------------
void test_visualization() {
    TEST("Visualization Output");

    Graph g(5);
    g.addEdge(0,1);
    g.addEdge(0,2);
    g.addEdge(1,3);
    g.addEdge(1,4);

    SpanningTree st;
    st.build(g, 0);

    std::string ascii = st.toASCII();
    std::string dot   = st.toDOT();
    std::string json  = st.toJSON();

    CHECK(!ascii.empty());
    CHECK(!dot.empty());
    CHECK(!json.empty());

    std::cout << "\nASCII Tree:\n" << ascii << "\n";
    std::cout << "DOT Format:\n" << dot << "\n";
    std::cout << "JSON:\n" << json << "\n";
}


// ------------------------------------------------------------
// 8. Stress Test (Async)
// ------------------------------------------------------------
void test_async_stress() {
    TEST("Async Stress Test");

    const int N = 200;
    Graph g(N);

    // Create a random tree
    std::mt19937 rng(1337);
    for (int i = 1; i < N; i++) {
        int parent = rng() % i;
        g.addEdge(parent, i);
    }

    SpanningTree st;
    st.build(g, 0);

    st.startAsync("stress async");
}


// ------------------------------------------------------------
// MAIN
// ------------------------------------------------------------
int main() {

    std::cout << "\n============================================\n";
    std::cout << "ASYNC SPANNING TREE TEST SUITE\n";
    std::cout << "============================================\n";

    test_single_node_async();
    test_simple_tree_async();
    test_async_convergecast_sum();
    test_multiple_async_runs();
    test_disconnected_graph_async();
    test_random_graph_async();
    test_visualization();
    test_async_stress();

    std::cout << "\n============================================\n";
    std::cout << "TEST SUMMARY\n";
    std::cout << "============================================\n";

    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    if (tests_failed == 0) {
        std::cout << "\nALL ASYNC TESTS PASSED\n";
        return 0;
    }

    std::cout << "\nASYNC TEST FAILURES DETECTED\n";
    return 1;
}
