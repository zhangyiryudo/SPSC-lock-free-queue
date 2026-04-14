#include <iostream>
#include <string>
#include "CircularQueue.h"
using namespace std;

int main() {
    CircularQueue<int> q(4);
    q.print();
    cout<<"---isEmpty " << q.isEmpty() << endl;
    q.enQueue(1);
    q.enQueuePowerOfTwo(2);
    cout<<"---isFull " << q.isFull() << endl;
    q.enQueuePowerOfTwo(3);
    cout<<"---isFull " << q.isFull() << endl;

    return 0;
}
