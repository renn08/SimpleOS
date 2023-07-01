#include "thread.h"
#include <ucontext.h>
#include <queue>
#include <cassert>
#include <unordered_map>
#include <iostream>
#include "cpu.h"
#include "cv.h"
#include "mutex.h"
#include "iostream"

using std::cout;
using std::endl;


std::unordered_map<int, char*> stack_map;
std::unordered_map<int, bool> finish_map;
// global dict for thread tcb
std::unordered_map<int, ucontext_t *> tid_map;
std::unordered_map<int, std::queue<ucontext_t*> > exit_wait_queue_map;
std::queue<ucontext_t*> finish_queue;
std::queue<int> finish_queue_index;

extern bool DEBUG;
extern ucontext_t* curr; 
extern int max_thread_id; // To maintain a non-duplicate key for the dict
extern ucontext_t *cpu_tcb;
extern std::queue<ucontext_t*> ready_queue; 

// for debug purpose
void see_ready_q();
void see_stack_map();

class thread::impl {
public:
    int tid;

    static void thread_begin(thread_startfunc_t body, void *arg, int thread_id, thread::impl* ptr);
    static void thread_exit(int thread_id, thread::impl* ptr);
    static void update_curr();
    static void update_ready_queue(int thread_id);
    static void add_exit_wait_queue(int thread_id);
    static void delete_finish();

    // constructor
    impl(thread_startfunc_t body, void *arg);
};

void thread::impl::delete_finish() {
    while (!finish_queue.empty()){
        // delete stack by deleting ptr record where the begining of the stack is when init
        delete[](char*) stack_map[finish_queue_index.front()];
        // also erase the key for the map
        stack_map.erase(finish_queue_index.front());
        delete finish_queue.front();
        tid_map.erase(finish_queue_index.front());
        finish_queue.pop();
        finish_queue_index.pop();
    }
}

thread::impl::impl(thread_startfunc_t body, void *arg) {
    delete_finish();

    ucontext_t *tcb;
    char *stack;

    try {
        tcb = new ucontext_t;
        stack = new char[STACK_SIZE];
        tcb->uc_stack.ss_sp = stack;
        tcb->uc_stack.ss_size = STACK_SIZE;
        tcb->uc_stack.ss_flags = 0;
        tcb->uc_link = nullptr;
    } catch (std::bad_alloc &err) {
        cpu::interrupt_enable();
        throw err;
    }
    
    // tcb makecontext
    makecontext(tcb, (void (*) ())thread_begin, 4, body, arg, max_thread_id, this);

    tid = max_thread_id;
    tid_map[max_thread_id] = tcb;
    stack_map[max_thread_id] = stack;
    finish_map[max_thread_id] = false;

    // put into the ready queue
    update_ready_queue(max_thread_id);

    max_thread_id++;
    if (curr == nullptr) {
        update_curr();
        setcontext(curr);
    }
}

void see_ready_q() {
    std::queue<ucontext_t*> q_copy = ready_queue;
    for (int i = 0; i < max_thread_id; i++)
            if (tid_map[i] == curr)
                std::cout << "curr: " << i << std::endl;
    while (!q_copy.empty())
    {
        ucontext_t* front = q_copy.front();
        for (int i = 0; i < max_thread_id; i++)
            if (tid_map[i] == front)
                std::cout << front << " " << i << std::endl;
        q_copy.pop();
    }
}

void see_stack_map() {
    for (auto const &pair: stack_map) {
        std::cout << "{" << pair.first << ": " << pair.second << "}\n";
    }
}

void thread::impl::thread_exit(int thread_id, thread::impl* ptr) {
    // exit-waiting queue->ready queue
    // destroy tcb
    assert_interrupts_enabled();
    cpu::interrupt_disable();
    add_exit_wait_queue(thread_id);
    
    delete_finish();

    finish_queue.push(curr);
    finish_queue_index.push(thread_id);

    // current thread finished, set curr to nullptr and update with the next thread on ready_q
    finish_map[thread_id] = true;
    curr = nullptr;
    update_curr();

    assert_interrupts_disabled();

    if (curr) setcontext(curr);
    assert(ready_queue.empty());
    // swap back to cpu::init
    setcontext(cpu_tcb);
}

void thread::impl::update_ready_queue(int thread_id) {
    assert(tid_map[thread_id]);
    ucontext_t* tcb = tid_map[thread_id];
    ready_queue.push(tcb);
}

void thread::impl::update_curr() {
    if (curr == nullptr) {
        if (!ready_queue.empty()) {
            ucontext_t* temp = ready_queue.front();
            std::swap(temp, curr);
            ready_queue.pop();
        } else {
            setcontext(cpu_tcb);
        }
    }
}

void thread::impl::add_exit_wait_queue(int thread_id) {
    while (!exit_wait_queue_map[thread_id].empty()) {
        ucontext_t* temp = exit_wait_queue_map[thread_id].front();
        exit_wait_queue_map[thread_id].pop();
        ready_queue.push(temp);
    }
    // after pour out all the thread in one thread's wait queue, delete this entry in the map
    exit_wait_queue_map.erase(thread_id);
}

void thread::impl::thread_begin(thread_startfunc_t body, void *arg, int thread_id, thread::impl* ptr) {
    cpu::interrupt_enable();
    body(arg);

    thread_exit(thread_id, ptr);
}

thread::thread(thread_startfunc_t body, void* arg){
    // create exit-waiting queue
    // create tcb
    assert_interrupts_enabled();
    cpu::interrupt_disable();

    try {
        this->impl_ptr = new impl(body, arg);
    } catch (std::bad_alloc &err) {
        cpu::interrupt_enable();
        throw err;
    }
    
    assert_interrupts_disabled();
    cpu::interrupt_enable();
}

thread::~thread() {
    // try disable and enable
    cpu::interrupt_disable();
    // erase the finish map
    finish_map.erase(this->impl_ptr->tid);
    delete this->impl_ptr;
    cpu::interrupt_enable();
}
void thread::join() {
    // current thread -> exit-wait queue
    assert_interrupts_enabled();
    cpu::interrupt_disable();
    
    // do normal join if the thread called to join is not finished, otherwise continue caller execution, join do nothing
    if (!finish_map[this->impl_ptr->tid]) {
        exit_wait_queue_map[this->impl_ptr->tid].push(curr);
        if (!ready_queue.empty()){
            curr = nullptr;
            this->impl_ptr->update_curr();
            swapcontext(exit_wait_queue_map[this->impl_ptr->tid].back(), curr);
        } else {
            // deal with the self-join situation (if the ready queue is empty and a thread is called to join, it can only be itself)
            setcontext(cpu_tcb);
        }
    }
    
    assert_interrupts_disabled();
    cpu::interrupt_enable();
}

void thread::yield() {
    // relinquish its CPU to the next ready thread if one exists; 
    // it does nothing if there are no ready threads.
    assert_interrupts_enabled();
    cpu::interrupt_disable();

    if (!ready_queue.empty()) {
        ready_queue.push(curr);
        ucontext_t* temp = ready_queue.front();
        std::swap(temp, curr);
        ready_queue.pop();
        swapcontext(ready_queue.back(), curr);
    }

    assert_interrupts_disabled();
    cpu::interrupt_enable();
} 
