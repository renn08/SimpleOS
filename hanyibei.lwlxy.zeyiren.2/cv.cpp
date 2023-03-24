#include "cv.h"
#include "cpu.h"
#include "impl.h"
#include "queue"
#include <ucontext.h>
#include <iostream>

using std::cout, std::endl;

extern ucontext_t* curr;
extern ucontext_t* cpu_tcb;
extern std::queue<ucontext_t*> ready_queue;
extern void see_ready_q();

bool DEBUG = false;


class cv::impl {
public:
    std::queue<ucontext_t*> cv_wait_queue;
};

cv::cv() {
    assert_interrupts_enabled();
    cpu::interrupt_disable();

    try {
        this->impl_ptr = new impl;
    } catch (std::bad_alloc &err) {
        cpu::interrupt_enable();
        throw err;
    }

    assert_interrupts_disabled();
    cpu::interrupt_enable();
}

cv::~cv() {
    delete this->impl_ptr;
}

void cv::wait(mutex& mtx) {
    assert_interrupts_enabled();
    cpu::interrupt_disable();

    mtx.impl_ptr->unlock();
    if (DEBUG) cout << "wait" << endl;
    this->impl_ptr->cv_wait_queue.push(curr);
    
    if (ready_queue.empty()){
        setcontext(cpu_tcb);
    }else{
        if (DEBUG) cout << "wake up" << endl;
        ucontext_t* temp = ready_queue.front();
        if (DEBUG) cout << "is it you" << endl;
        ready_queue.pop();
        if (DEBUG) cout << "no" << endl;
        std::swap(curr, temp);
        swapcontext(this->impl_ptr->cv_wait_queue.back(), curr);
    }
    if (DEBUG) cout << "hello" << endl;

    mtx.impl_ptr->lock();
    if (DEBUG) cout << "it is me" << endl;

    assert_interrupts_disabled();
    cpu::interrupt_enable();
} // wait on this condition variable

void cv::signal() {
    assert_interrupts_enabled();
    cpu::interrupt_disable();
    if (DEBUG) cout << "signal" << endl;
    if (! this->impl_ptr->cv_wait_queue.empty()){
        ucontext_t* temp = impl_ptr->cv_wait_queue.front();
        this->impl_ptr->cv_wait_queue.pop();
        ready_queue.push(temp);
    }
    if (DEBUG) cout << "signal over" << endl;

    assert_interrupts_disabled();
    cpu::interrupt_enable();
} // wake up one thread on this condition variable

void cv::broadcast() {
    assert_interrupts_enabled();
    cpu::interrupt_disable();

    while (! this->impl_ptr->cv_wait_queue.empty()){
        ucontext_t* temp = impl_ptr->cv_wait_queue.front();
        this->impl_ptr->cv_wait_queue.pop();
        ready_queue.push(temp);
    }
    if (DEBUG) cout << "broadcast over" << endl;

    assert_interrupts_disabled();
    cpu::interrupt_enable();
}   // wake up all threads on this condition variable
    