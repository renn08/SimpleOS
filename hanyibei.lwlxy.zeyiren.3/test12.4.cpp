// test vm_destroy
#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

void parent() {
    if (fork()) {
        vm_yield();
        cout << "HERE2222" << endl;
    } else {
        cout << "NOT HERE222" << endl;
        char* p2 = (char *) vm_map(nullptr, 0);
        strcpy(p2, "data1.bin");
        char* p3 = (char*) vm_map(p2, 0);
        for (unsigned int i=0; i<32; i++) {
            cout << p3[i];
        }
    }
}

int main() {
    parent();
    char* filename = (char *) vm_map(nullptr, 0);
    strcpy(filename, "data1.bin");
    char* p = (char *) vm_map(filename, 0);
    for (unsigned int i=0; i<32; i++) {
        cout << p[i];
    }
    return 0;
}