// Huffman encoding test utility - builds a Huffman tree and generates compression routes
#include <stdio.h>
#include <stdlib.h>
#include "Node.h"
#include "Tree.h"

int main() {
    // Initialize priority queue for building the Huffman tree
    priority_queue pq;
    initPriorityQueue(&pq);

    int n, f;
    uchar c;
    
    // Read the number of unique bytes/characters to process
    scanf("%d", &n);
    
    // Read frequency and character pairs, create nodes, and add to priority queue
    while(n--){
        scanf("%d %c", &f, &c);
        push(&pq, createNode(f, c));
    }

    // Build the Huffman tree from the priority queue
    node* tree = makeTree(&pq);
    displayTree(tree, "", false);

    // Generate and display the Huffman codes for each byte
    // routes[i] contains the encoding path for byte i
    // For example, route['a'] = {length: 5, bitmask: 10110}
    route** routes = makeRoutes(tree);
    for(int i = 0; i<MAX; ++i)
        if(routes[i])
            displayRoute(routes[i], i);

    return 0;
}
