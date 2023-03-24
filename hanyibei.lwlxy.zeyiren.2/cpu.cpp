#include "cpu.h"
#include <ucontext.h>
#include <iostream>
#include <unordered_map>
#include <queue>
#include "cv.h"
#include "mutex.h"
#include "thread.h"

using std::cout, std::endl;

ucontext_t *cpu_tcb;     
int max_thread_id = 0;
ucontext_t* curr = nullptr;  
std::queue<ucontext_t*> ready_queue;

extern bool DEBUG;
extern std::queue<ucontext_t*> finish_queue;
extern std::unordered_map<int, char*> stack_map;


void timer_interrupt_handler() {
    assert_interrupts_enabled();
    thread::yield();
    assert_interrupts_enabled();
}

void dummy_create_thread(thread_startfunc_t body, void* arg) {
    cpu::interrupt_enable();
    thread(body, arg);
}

void cpu::init(thread_startfunc_t body, void* arg) {
    assert_interrupts_disabled();
    interrupt_vector_table[cpu::TIMER] = timer_interrupt_handler;
    // cpu::interrupt_enable();

    ucontext_t *tcb_first;
    try {
        tcb_first = new ucontext_t;
        // tcb initialization
        char *stack = new char[STACK_SIZE];
        tcb_first->uc_stack.ss_sp = stack;
        tcb_first->uc_stack.ss_size = STACK_SIZE;
        tcb_first->uc_stack.ss_flags = 0;
        tcb_first->uc_link = nullptr;
        stack_map[max_thread_id] = stack;
        max_thread_id++;
    } catch (std::bad_alloc &err) {
        cpu::interrupt_enable();
        throw err;
    }
    
    makecontext(tcb_first, (void (*) ())dummy_create_thread, 2, body, arg);
    
    try {
        cpu_tcb = new ucontext_t;
        char *stack1 = new char[STACK_SIZE];
        cpu_tcb->uc_stack.ss_sp = stack1;
        cpu_tcb->uc_stack.ss_size = STACK_SIZE;
        cpu_tcb->uc_stack.ss_flags = 0;
        cpu_tcb->uc_link = nullptr;
    } catch(std::bad_alloc &err) {
        cpu::interrupt_enable();
        throw err;
    }
    
    swapcontext(cpu_tcb, tcb_first);

    // delete the dummy thread tcb and stack
    delete[](char*) stack_map[0];
    delete tcb_first;

    while (!finish_queue.empty()){
        delete finish_queue.front();
        finish_queue.pop();
    }
    cpu::interrupt_enable_suspend();
}
