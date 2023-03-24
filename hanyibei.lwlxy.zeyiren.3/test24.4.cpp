#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using namespace std;

int main()
{   
    cout << "1\n";
    if (fork()) {
        if (fork ()) {
            cout << "parent parent 1\n";
            char *filename = (char *) vm_map(nullptr, 0);
            strcpy(filename,  "lampson83.txt");
            char *p = (char*) vm_map(filename, 0);
            for (unsigned int i=0; i<32; i++) {
                p[i] = 'a';
            }
        } else {
            cout << "parent child\n";
            char *filename = (char *) vm_map(nullptr, 0);
            strcpy(filename,  "lampson83.txt");
            char *p = (char*) vm_map(filename, 0);
            for (unsigned int i=0; i<42; i++) {
                cout << p[i];
            }
            cout << endl;
        }
        cout << "parent 1\n";
    } else {
        cout << "child 1\n";
        fork();
        char *filename = (char *) vm_map(nullptr, 0);
        strcpy(filename,  "lampson83.txt");
        char *p = (char*) vm_map(filename, 0);
        for (unsigned int i=0; i<42; i++) {
            cout << p[i];
        }
        cout << endl;
        vm_yield();
        cout << "child 1 again\n";
    }
    vm_yield();
    cout << "2\n";
    if (fork()) {
        cout << "parent 3\n";
        char *filename = (char *) vm_map(nullptr, 0);
        strcpy(filename,  "lampson83.txt");
        char *p = (char*) vm_map(filename, 0);
        for (unsigned int i=0; i<32; i++) {
            p[i] = 'b';
        }
    } else {
        cout << "child 3\n";
        fork();
        char *filename = (char *) vm_map(nullptr, 0);
        strcpy(filename,  "lampson83.txt");
        char *p = (char*) vm_map(filename, 0);
        for (unsigned int i=0; i<42; i++) {
            cout << p[i];
        }
        cout << endl;
        cout << "child 3 again\n";
    }
    fork();
    cout << "4\n";
    vm_yield();
    if (fork()) {
        cout << "parent 4\n";
        char *filename = (char *) vm_map(nullptr, 0);
        strcpy(filename,  "lampson83.txt");
        char *p = (char*) vm_map(filename, 0);
        for (unsigned int i=0; i<32; i++) {
            p[i] = 'c';
        }
    } else {
        cout << "child 1\n";
        fork();
        char *filename = (char *) vm_map(nullptr, 0);
        strcpy(filename,  "lampson83.txt");
        char *p = (char*) vm_map(filename, 0);
        for (unsigned int i=0; i<42; i++) {
            cout << p[i];
        }
        cout << endl;
        vm_yield();
        cout << "child 1 again\n";
    }

    char *filename = (char *) vm_map(nullptr, 0);
    strcpy(filename,  "lampson83.txt");
    char *p = (char*) vm_map(filename, 0);
    for (unsigned int i=0; i<42; i++) {
        cout << p[i];
    }
    cout << endl;
    return 0;
}   