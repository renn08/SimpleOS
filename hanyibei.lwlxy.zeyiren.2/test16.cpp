// test complex situation
#include <iostream>
#include <cstdlib>
#include "thread.h"

using std::cout;
using std::endl;

extern void see_ready_q();

int num = 0;
thread *thread_ptr;

mutex m1;
cv cv1;

void child(void *a)
{
    int id = (intptr_t) a;

    m1.lock();
    cout << "child " << id << endl;
    thread::yield();
    cout << id<<"end" << endl;
    m1.unlock();
}

void child2(void *a)
{
    int id = (intptr_t) a;
    cout << "enter child " << id << endl;
    thread::yield();
    cout<< "re-enter child " << id << endl;
    m1.lock();
    cout << "child " << id << endl;
    cout << id << "end" << endl;
    num=1;
    
    m1.unlock();
}

void parent(void *a)
{
    int id = (intptr_t) a;

    m1.lock();
    cout << "parent called with " << id << endl;
    m1.unlock();

    thread t1 ( (thread_startfunc_t) child, (void *) 1);
    thread t2 ( (thread_startfunc_t) child2, (void *) 2);
    thread_ptr = &t2;
    cout<<"parent yield"<<endl;
    // see_ready_q();
    thread::yield();
    // see_ready_q();
    cout<<"parent yield back"<<endl;

    t2.join();
    cout << "t2 join" << endl;
    thread_ptr->join();
}

int main()
{
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 100, false, false, 0);
}