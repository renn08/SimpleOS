// Deadlock with two threads joining each other
#include <iostream>
#include <cstdlib>
#include "thread.h"

using std::cout, std::endl;

// extern void see_ready_q();

mutex m1, m2;
cv c1;
int num;
thread* t1_t;
thread* t2_t;

void child(void* b){
    int id = (intptr_t) b;
    cout<< "Child "<< id << endl;
    t1_t->join();
    cout<< "Child "<< id <<" get unlock "<< num <<endl;
}

void child3(){
    cout<< "Child "<< "fake parent" <<endl;
    // see_ready_q();
    thread t2 ((thread_startfunc_t) child, (void *) 2);
    t2_t = &t2;
    cout<< "Fake "<<" yield "<< num << endl;
    // see_ready_q();
    thread::yield();
    cout<< "Fake " <<" yield back "<< num << endl;
    t2_t->join();
    cout << "T2 Destroyed" << endl;
}

void parent(void* a){
    cout << "Parent start" << endl;
    thread t1 ((thread_startfunc_t) child3, (void *) 1);
    t1_t = & t1;
    cout << "Child thread created" << endl;
    thread::yield();
    cout <<"Second parent yield to child 2" << endl;
    thread::yield();
    cout << "Child 2 joined" << endl;
    cout << "Parent finish"<<endl;
}

int main(){
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 0, false, false, 0);
}

