//Test thread impl deleted (may error)
#include <iostream>
#include <cstdlib>
#include "thread.h"

using std::cout, std::endl;

mutex m1;

void parent(void* a){
    try {
        m1.unlock();
    } catch (std::runtime_error &err) {
        cout << "unlock not owned lock" <<endl;
    }
  
}

int main(){
    cpu::boot(1, (thread_startfunc_t) parent, (void *) 0, false, false, 0);
}