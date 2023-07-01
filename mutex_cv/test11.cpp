// signaling without holding the mutex
// using waiting without while
#include <iostream>
#include "thread.h"

using std::cout;
using std::endl;

mutex m1;
cv cv1;

int child_done = 0;

void child(void *a) {
    char *message = (char *) a;
    // make the child go to the end of the ready_queue
    thread::yield();
    m1.lock();
    cout << "child called with msg " << message << ", child_done = 1" << endl;
    child_done = 1;
    cv1.signal();
    m1.unlock();
}

void child2(void *a) {
    cout << "child2 wild signal without holding the mutex" << endl;
    cv1.signal();
}

void parent(void *a)
{
    intptr_t arg = (intptr_t) a;
    m1.lock();
    cout << "parent called " << arg << endl;
    m1.unlock();

    thread t2 ((thread_startfunc_t) child2, (void *) 0);
    thread t1 ((thread_startfunc_t) child, (void *) "test message");

    m1.lock();
    
    if (!child_done) {
        cout << "parent waiting for child\n";
        cv1.wait(m1);
    }

    cout << "parent finish" << endl;
    m1.unlock();
}

int main()
{
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 100, false, false, 0);
}