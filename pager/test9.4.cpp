#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

int main(){
    if (fork()) {
        char* page0 = (char*) vm_map(nullptr, 0);
        cout << "page0 create" << endl;
        vm_yield();
        char* page1 = (char*) vm_map(nullptr, 0);
        cout << "page1 create" << endl;
        char* page2 = (char*) vm_map(nullptr, 0);
        cout << "page2 create" << endl;
        page0[0] = page1[0] = page2[0] = 'a';
    } else {
        char* page0 = (char*) vm_map(nullptr, 0);
        strcpy(page0, "HELLO WORLD!");
        cout << "hello world" << endl;
    }
}