// error handling too much memory

#include <iostream>
#include <cstdlib>
#include "thread.h"
#include <queue>

using std::cout;
using std::endl;

extern void see_ready_q();

mutex m1;
cv c1;
int num;
std::queue<thread *> thread_comb;


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
    m1.unlock();
    cout<<"Child "<<id<<" get unlock "<<num<<endl;
}

void parent(void* a){
    cout<<"Parent start"<<endl;
    try{
        for (int i=0; i<5000; i++){
            intptr_t id = i;
            thread *t = new thread((thread_startfunc_t) child, (void *) id);
            thread_comb.push(t);
        }
    }catch(std::bad_alloc &err){
        throw err;
    }
    cout<<"Parent finish"<<endl;
}

int main(){
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 0, false, false, 0);
}