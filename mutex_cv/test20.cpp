// error handling unlock other's mutex

#include <iostream>
#include <cstdlib>
#include "thread.h"

using std::cout;
using std::endl;

// extern void see_ready_q();

mutex m1;
cv c1;
int num;

void child(void* b){
    int id = (intptr_t) b;
    cout<<"Child "<<id<<" get lock "<<num<<endl;
    // see_ready_q();
    m1.lock();
    num+=1;
    cout<<"Child "<<id<<" get num "<<num<<endl;
    // cout<<"Child "<<id<<" yield "<<num<<endl;
    // thread::yield();
    // cout<<"Child "<<id<<" yield back "<<num<<endl;
    num-=1;
    cout<<"Child "<<id<<" get num "<<num<<endl;
    // try {
        m1.unlock();
        // } catch (...) {
        // cout << "hello" <<endl;
    // }
    
    cout<<"Child "<<id<<" get unlock "<<num<<endl;
}

void child2(void* b){
    int id = (intptr_t) b;
    cout<<"Child "<<id<<" get lock "<<num<<endl;
    // see_ready_q();
    // try {
    //     m1.unlock();
    // } catch (...) {cout << "hello" <<endl;}
    num+=3;
    cout<<"Child "<<id<<" get num "<<num<<endl;
    cout<<"Child "<<id<<" yield "<<num<<endl;
    // see_ready_q();
    thread::yield();

    cout<<"Child "<<id<<" yield back"<<num<<endl;
    num-=3;
    cout<<"Child "<<id<<" get num "<<num<<endl;
    try {
    m1.unlock(); 
    }
    catch(std::runtime_error & err) {
        cout << "hello" <<endl;
    }
    cout<<"Child "<<id<<" get unlock "<<num<<endl;
}

void parent(void* a){
    cout<<"Parent start"<<endl;
    try {
    m1.unlock(); 
    }
    catch(std::runtime_error & err) {
        cout << "hello" <<endl;
    }
    thread t1 ((thread_startfunc_t) child, (void *) 1);
    thread t2 ((thread_startfunc_t) child2, (void *) 2);
    thread t3 ((thread_startfunc_t) child2, (void *) 3);
    thread t4 ((thread_startfunc_t) child2, (void *) 4);
    cout << "Child thread created" << endl;
    cout<<"Parent finish"<<endl;
}

int main(){
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 0, false, false, 0);
}