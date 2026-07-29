/*This is a convenience header for the SpanningTree class
Simply include this header and you can use the SpanningTree.
the following is a usage example/Test;

#include "SpanningTree.hpp"
#include <iostream>
int main() {
    // Create a graph with 7 nodes
    // Topology: 0 is root. 0 connected to 1,2. 1 to 3,4. 2 to 5,6.
    Graph network(7);
    
    // Layer 1
    network.addEdge(0, 1);
    network.addEdge(0, 2);
    
    // Layer 2
    network.addEdge(1, 3);
    network.addEdge(1, 4);
    network.addEdge(2, 5);
    network.addEdge(2, 6);
    
    // Add a redundant "cycle" edge (The BFS should ignore this for the tree)
    network.addEdge(3, 4); 
    std::cout<<"Printing Graph: \n";
network.printGraph();

    std::cout << "--- Building Spanning Tree ---\n";
    SpanningTree st;
    st.buildFromGraph(network, 0);

    std::cout << "\n--- Starting Protocol ---\n";
    st.start("INIT_SIGNAL");

    st.start("TEST");
network.printGraph();

    return 0;
}

*/

#include "SpanningTree/node.hpp"
#include "SpanningTree/graph.hpp"
#include "SpanningTree/spanning_tree.hpp"
