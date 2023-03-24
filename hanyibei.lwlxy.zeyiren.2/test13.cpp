// exit without unlock

#include <iostream>
#include "thread.h"

using std::cout;
using std::endl;

mutex m1;

int num = 0;

int child_done = 0;

void child(void *a) {
    // char *message = (char *) a;
    // make the child go to the end of the ready_queue
    thread::yield();
    cout<<"yield back"<<endl;
    m1.lock();
    cout << "child" << endl;
    if (num==2) {child_done = 1;}
    num += 1;
}

void parent(void *a)
{
    intptr_t arg = (intptr_t) a;
    m1.lock();
    cout << "parent called " << arg << endl;
    m1.unlock();

    thread t1 ((thread_startfunc_t) child, (void *) "test message");
    thread t2 ((thread_startfunc_t) child, (void *) "test message");


    cout << "parent waiting for child\n";

    cout << "parent finish" << endl;
    cout<<num<<endl;
}

int main()
{
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 100, false, false, 0);
}