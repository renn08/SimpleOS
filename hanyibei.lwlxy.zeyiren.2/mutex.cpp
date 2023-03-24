// #define _XOPEN_SOURCE
#include "mutex.h"
#include "cpu.h"
#include "cv.h"
#include "impl.h"
#include <ucontext.h>
#include <iostream>
#include <queue>
#include "cpu.h"

using std::cout, std::endl;

extern bool DEBUG;
extern ucontext_t* curr;
extern ucontext_t* cpu_tcb;
extern std::queue<ucontext_t*> ready_queue;


mutex::mutex() {
    assert_interrupts_enabled();
    cpu::interrupt_disable();

    try {
        this->impl_ptr = new impl;
    } catch(std::bad_alloc &err) {
        cpu::interrupt_enable();
        throw err;
    }

    assert_interrupts_disabled();
    cpu::interrupt_enable();
}

mutex::~mutex() {
    delete this->impl_ptr;
}

void mutex::lock() {
    // while (free. 0->wait queue swap-to-next.) 1->free=0
    assert_interrupts_enabled();
    cpu::interrupt_disable();

    this->impl_ptr->lock();

    assert_interrupts_disabled();
    cpu::interrupt_enable();
}

void mutex::unlock() {
    // handoff lock!!! free=1. deQ(wait_q) enQ(ready_q) free=0
    assert_interrupts_enabled();
    cpu::interrupt_disable();

    try {
        this->impl_ptr->unlock();
    } catch (std::runtime_error &err) {
        cpu::interrupt_enable();
        throw err;
    }
    
    
    assert_interrupts_disabled();
    cpu::interrupt_enable();
}

void mutex::impl::unlock() {
    if (!this->free && this->owner == curr) {
        this->free = true;
        this->owner = nullptr;
        if (!this->mutex_wait_queue.empty()) {
            ucontext_t* next = this->mutex_wait_queue.front();
            this->mutex_wait_queue.pop();
            ready_queue.push(next);
            this->free = false;
            this->owner = next;
        }
    } else {
        // throw runtime error if the owner of the lock where unlock is called is not the caller of the unlock function 
        // or the lock is not even held by anyone.
        cpu::interrupt_enable();
        throw std::runtime_error("Unlock other's mutex\n");
    }
}

void mutex::impl::lock() {
    if (!this->free) {
        this->mutex_wait_queue.push(curr);
        if (!ready_queue.empty()) {
            ucontext_t* next = ready_queue.front();
            ready_queue.pop();
            std::swap(curr, next);
            swapcontext(this->mutex_wait_queue.back(), curr);
        } else {
            setcontext(cpu_tcb);
        }
    } else {
        this->owner = curr;
        this->free = false;
    }
}
