// test if vm_destroy deleted the swap backed blocks
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
        for (unsigned int i=0; i<12; i++) {
            p3[i] = 'o';
        }
        cout << endl;
    }
}

int main() {
    char* filename2 = (char *) vm_map(nullptr, 0);
    strcpy(filename2, "data1.bin");
    char* p4 = (char *) vm_map(filename2, 0);
    for (unsigned int i=0; i<32; i++) {
        cout << p4[i];
    }
    cout << "parent()" << endl;
    parent();
    cout << "parent() next line" << endl;
    char* filename = (char *) vm_map(nullptr, 0);
    strcpy(filename, "data1.bin");
    char* p = (char *) vm_map(filename, 0);
    for (unsigned int i=0; i<32; i++) {
        cout << p[i];
    }
    cout << endl;
    cout << p4 << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p4[i];
    }
    return 0;
}