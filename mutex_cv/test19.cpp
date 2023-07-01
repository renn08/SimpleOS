//Test thread impl deleted (may error)
#include <iostream>
#include <cstdlib>
#include "thread.h"

using std::cout, std::endl;

// extern void see_ready_q();

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
    thread* t2 =  new thread((thread_startfunc_t) child, (void *) 2);
    t1_t = t2;
    cout<< "Fake "<<" yield "<< num << endl;
    // see_ready_q();
    thread::yield();
    cout<< "Fake " <<" yield back "<< num << endl;
    delete t2;
    cout << "T2 Destroyed" << endl;
}

void parent(void* a){
    cout << "Parent start" << endl;
    thread t1 ((thread_startfunc_t) child3, (void *) 1);
    cout << "Child thread created" << endl;
    thread::yield();
    cout <<"Second parent yield" << endl;
    thread::yield();
    // child 3 after yield will run and delete object thread t2
    cout << "Join child 2" <<endl;
    cout << t1_t->impl_ptr << endl; // should be null but not
    // t1_t->impl_ptr = nullptr;
    // cout << t1_t->impl_ptr << endl;
    t1_t->join(); // why this can still be called, the autograder said it exposed a bug in our code
    cout << "Child 2 joined" << endl;
    cout << "Parent finish"<<endl;
}

int main(){
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 0, false, false, 0);
}

