#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

void parent() {
    if (fork()) {
        vm_yield();
        cout << "HERE111111" << endl;
        if (fork()) {
            vm_yield();
            cout << "HERE3333333" << endl;
        } else {
            cout << "NOT HERE3333333" << endl;
        }
    } else {
        cout << "NOT HERE111111" << endl;
        if (fork()) {
            vm_yield();
            cout << "HERE44444444" << endl;
        } else {
            cout << "NOT HERE444444444" << endl;
        }
    }
    if (fork()) {
        vm_yield();
        cout << "HERE2222" << endl;
    } else {
        cout << "NOT HERE222" << endl;
    }
}

int main() {
    parent();
    char* p = (char *) vm_map(nullptr, 0);
    for (unsigned int i=0; i<32; i++) {
        cout << p[i];
    }
    return 0;
}