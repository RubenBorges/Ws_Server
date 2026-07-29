#include "SpanningTree.hpp"
#include <cassert>
#include <iostream>

static int passed = 0, failed = 0;

#define TEST(name) std::cout << "\n[TEST] " << name << "\n"
#define CHECK(cond) \
do { \
        if (!(cond)) { \
            std::cerr << "FAILED: " #cond " at line " << __LINE__ << "\n"; \
            failed++; \
    } else passed++; \
} while (0)

    void test_single_node() {
    TEST("Single Node Tree");

    Graph g(1);
    SpanningTree st;
    st.build(g, 0);

    CHECK(st.getRoot()->id == 0);
    CHECK(st.getRoot()->isLeaf());

    st.start("hello");
}

void test_simple_tree() {
    TEST("Simple Tree");

    Graph g(4);
    g.addEdge(0,1);
    g.addEdge(0,2);
    g.addEdge(1,3);

    SpanningTree st;
    st.build(g, 0);

    Node* r = st.getRoot();
    CHECK(r->id == 0);
    CHECK(r->children.size() == 2);

    st.start("msg");
}

void test_convergecast_sum() {
    TEST("Convergecast Sum");

    Graph g(5);
    g.addEdge(0,1);
    g.addEdge(0,2);
    g.addEdge(1,3);
    g.addEdge(1,4);

    SpanningTree st;
    st.build(g, 0);

    // Expected sum = sum(id*10) = 0+10+20+30+40 = 100
    st.start("compute");
}

void test_multiple_runs() {
    TEST("Multiple Runs");

    Graph g(3);
    g.addEdge(0,1);
    g.addEdge(1,2);

    SpanningTree st;
    st.build(g, 0);

    st.start("first");
    st.start("second");
}

void test_disconnected_graph() {
    TEST("Disconnected Graph");

    Graph g(5);
    g.addEdge(0,1);
    g.addEdge(3,4);

    SpanningTree st;
    st.build(g, 0);

    CHECK(st.getNode(2) == nullptr);
    CHECK(st.getNode(3) == nullptr);
}

void test_random_graph() {
    TEST("Random Graph");

    Graph g(50);
    for (int i = 0; i < 49; i++)
        g.addEdge(i, i+1);

    SpanningTree st;
    st.build(g, 0);

    st.start("random");
}

int main() {
    test_single_node();
    test_simple_tree();
    test_convergecast_sum();
    test_multiple_runs();
    test_disconnected_graph();
    test_random_graph();

    std::cout << "\nPassed: " << passed << "\nFailed: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}
