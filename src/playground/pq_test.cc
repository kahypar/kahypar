#include <iostream>
#include <array>
#include "../lib/datastructure/PriorityQueue.h"

int main(int argn, char **argv) {
typedef datastructure::PriorityQueue<int, int, std::numeric_limits<int> > RefinementPQ;
std::cout << "test:::" << std::endl;
    RefinementPQ* q1 = new RefinementPQ(4876);
    RefinementPQ* q2 = new RefinementPQ(4876);

    q1->insert(4900,2);
    q2->insert(4900,5000);
    
    delete q1;
    delete q2;
    std::cout << "::::test" << std::endl;
}
