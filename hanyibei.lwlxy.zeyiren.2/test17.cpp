//  Lock again

#include <iostream>
#include <cstdlib>
#include "thread.h"
#include "cpu.h"

using std::cout;
using std::endl;

int num = 0;
mutex m1, m2;
cv cv1;

void child(void *b){
    int id = (intptr_t) b;
    m1.lock();
    m1.lock();
    assert_interrupts_enabled();
    cout<<"child "<<id<<" locked"<<endl;
    num += 1;
    cout<<"child "<<id<<" get num "<<num<<endl;
    m1.unlock();
    cout<<"child "<<id<<" finished"<<endl;
}

void child2(void *b){
    int id = (intptr_t) b;
    m2.lock();
    assert_interrupts_enabled();
    cout<<"child "<<id<<" locked"<<endl;
    num += 1;
    cout<<"child "<<id<<" get num "<<num<<endl;
    m2.unlock();
    cout<<"child "<<id<<" finished"<<endl;
}

void parent(void *a)
{
    m1.lock();
    cout<<"parent begin"<<endl;
    thread t1 ( (thread_startfunc_t) child, (void *) 1);
    thread t2 ( (thread_startfunc_t) child2, (void *) 2);
    thread t3 ( (thread_startfunc_t) child, (void *) 3);
    thread t4 ( (thread_startfunc_t) child, (void *) 4);
    num += 1;
    cout<<"Thread created"<<endl;
    m1.unlock();
    cout<<"Parent finished!"<<endl;
}

int main()
{
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 100, false, false, 0);
}