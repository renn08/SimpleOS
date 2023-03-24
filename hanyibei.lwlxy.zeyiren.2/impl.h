// in order to make call this->impl_ptr->lock and unlock, no definition otherwise
#ifndef _IMPL_H
#define _IMPL_H
#include "mutex.h"
#include "cpu.h"
#include "cv.h"
#include "impl.h"
#include <ucontext.h>
#include <iostream>
#include <queue>
#include "cpu.h"

extern ucontext_t* curr;

// Mutex
class mutex::impl {
public:
    impl() : owner(curr), free (true) {};
    void unlock();
    void lock();
    ucontext_t *owner = nullptr;
    bool free;
    std::queue<ucontext_t*> mutex_wait_queue;
};

#endif