// Virtual page with same (filename, block)
// Test clock queue eviction

#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

void parent() {
    if (fork()) {
        cout << "parent\n";
        vm_yield();
        // Another swap-backed page in a new process
        char * filename = (char *)vm_map(nullptr, 0);
        strcpy(filename, "data1.bin");
        cout << filename << endl;
    } else {
        cout << "child\n";
        char * filename = (char *)vm_map(nullptr, 0);
        strcpy(filename, "data1.bin");
        cout << filename << endl;
        for (int i = 0; i < 255; i++) {
            cout << "===========================" << i << endl;
            char *p = (char *) vm_map (filename, 0);
            // /* Print the first part of the paper */
            for (unsigned int j=0; j<32; j++) {
                cout << p[j];
            }
            cout << endl;
        }
    }
}

int main()
{
    parent();
    cout << "finish\n";
    return 0;
}