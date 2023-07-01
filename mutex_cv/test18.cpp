//Test thread impl deleted (may error)
#include <iostream>
#include <cstdlib>
#include "thread.h"

using std::cout, std::endl;

// extern void see_ready_q();
// extern void see_stack_map();

mutex m1, m2;
cv c1;
int num;
thread* t1_t;

void child(void* b){
    int id = (intptr_t) b;
    cout<<" Child "<< id <<" get lock "<< num << endl;
    cout<< "Child "<< id <<" yield "<< num << endl;
    thread::yield();
    cout<< "Child "<< id <<" yield back "<< num << endl;
    m1.lock();
    num += 1;
    cout<< "Child "<< id <<" get num "<< num << endl;
    // cout<< "Child "<< id <<" yield "<< num << endl;
    // see_ready_q();
    // thread::yield();
    // cout<< "Child "<< id <<" yield back "<< num << endl;
    num-=1;
    cout<< "Child "<< id <<" get num "<< num << endl;
    m1.unlock();
    cout<< "Child "<< id <<" get unlock "<< num <<endl;
}

void child3(){
    cout<< "Child "<< "fake parent" <<endl;
    // see_ready_q();
    thread t2 ((thread_startfunc_t) child, (void *) 2);
    t1_t = &t2;
    cout<< "Fake "<<" yield "<< num << endl;
    // see_ready_q();
    thread::yield();
    cout<< "Fake " <<" yield back "<< num << endl;
    cout << "T2 Destroyed" << endl;
}

void parent(void* a){
    cout << "Parent start" << endl;
    thread t1 ((thread_startfunc_t) child3, (void *) 1);
    cout << "Child thread created" << endl;
    // thread::yield();
    // cout <<"Second parent yield" << endl;
    // see_stack_map();
    thread::yield();
    cout << "Join child 2" <<endl;
    t1_t->join();
    cout << "Child 2 joined" << endl;
    cout << "Parent finish"<<endl;
    // see_stack_map();
}

int main(){
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 0, false, false, 0);
}

