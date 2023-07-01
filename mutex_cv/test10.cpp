// multiple thread join, test caller continue execution if thread a is finished but call a.join()
#include <iostream>
#include <cstdlib>
#include "thread.h"

using std::cout, std::endl;

// extern void see_ready_q();

mutex m1, m2;
cv c1;
int num;

void child(void* b){
    int id = (intptr_t) b;
    cout<<" Child "<< id <<" get lock "<< num << endl;
    // see_ready_q();
    m1.lock();
    num += 1;
    cout<< "Child "<< id <<" get num "<< num << endl;
    cout<< "Child "<< id <<" yield "<< num << endl;
    thread::yield();
    cout<< "Child "<< id <<" yield back "<< num << endl;
    num-=1;
    cout<< "Child "<< id <<" get num "<< num << endl;
    m1.unlock();
    cout<< "Child "<< id <<" get unlock "<< num <<endl;
}

void child2(void* b){
    int id = (intptr_t) b;
    cout<< "Child "<< id <<" get lock "<< num <<endl;
    // see_ready_q();
    m2.lock();
    num += 3;
    cout << "Child "<< id <<" get num "<< num <<endl;
    cout << "Child "<< id <<" yield "<< num <<endl;
    // see_ready_q();
    thread::yield();

    cout << "Child "<< id <<" yield back"<< num <<endl;
    num-=3;
    cout << "Child "<< id <<" get num "<< num <<endl;
    m2.unlock();
    cout << "Child "<< id <<" get unlock "<< num <<endl;
}

void child3(void* b){
    thread* t = (thread*) b;
    cout<< "Child "<< "fake parent" << num <<endl;
    // see_ready_q();
    t->join();
    cout << "after join" << endl;
    // see_ready_q();
    thread::yield();

}

void parent(void* a){
    cout << "Parent start" << endl;
    thread t1 ((thread_startfunc_t) child, (void *) 1);
    thread* t1_t = &t1;
    thread t3 ((thread_startfunc_t) child2, (void *) 3);
    thread t4 ((thread_startfunc_t) child2, (void *) 4);
    t1.join();
    thread t2 ((thread_startfunc_t) child3, (void *) t1_t);

    cout << "Child thread created" << endl;

    cout << "Child 1 joined" << endl;
    t2.join();
    cout << "Child 2 joined" << endl;
    cout << "Parent finish"<<endl;
}

int main(){
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 0, false, false, 0);
}