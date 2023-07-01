//
// lock unlock yield
//
#include "cpu.h"
#include "cv.h"
#include "thread.h"
#include "mutex.h"
#include <iostream>
#include <cstdlib>

using std::cout;
using std::endl;

// extern void see_ready_q();

int g = 0;

mutex mutex1;
cv cv1;

void loop(void *a)
{
    char *id = (char *) a;
    int i;
    cout << "thread: " << id << endl;

    mutex1.lock();
    cout << "loop called with id " << id << endl;

    for (i=0; i<5; i++, g++) {
	    cout << id << ":\t" << i << "\t" << g << endl;
        // see_ready_q();
        mutex1.unlock();
	    thread::yield();
        mutex1.lock();
    }
    cout << id << ":\t" << i << "\t" << g << endl;
    mutex1.unlock();
}

void parent(void *a)
{
    cout << "parent" <<endl;
    intptr_t arg = (intptr_t) a;

    mutex1.lock();
    cout << "parent called with arg " << arg << endl;
    mutex1.unlock();
    cout << "unlock" << endl;

    thread t1 ( (thread_startfunc_t) loop, (void *) "child thread");
    cout << "hello" << endl;

    cout << "start parent thread loop" << endl;
    loop( (void *) "parent thread");
}

int main()
{
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 100, false, false, 0);
}