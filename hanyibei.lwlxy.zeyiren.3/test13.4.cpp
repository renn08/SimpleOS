#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

// test for evict
int main() {
    cout << "main start!" << endl;

    cout << "swap-back page!" << endl;
    char *filename = (char *)vm_map(nullptr, 0);
    strcpy(filename, "lampson83.txt");
    cout << "file-back page for lampson83!" << endl;
    char *p1 = (char *)vm_map(filename, 0);

    cout << "swap-back page!" << endl;
    char *filename_2 = (char *)vm_map(nullptr, 0);
    strcpy(filename_2, "data1.bin");
    cout << "file-back page for data1!" << endl;
    char *p2 = (char *)vm_map(filename_2, 0);

    for (unsigned int i=0; i<32; i++) {
        cout << p1[i];
    }
    cout << endl;

    cout << "swap-back page!" << endl;
    char *filename_3 = (char *)vm_map(nullptr, 0);
    strcpy(filename_3, "data2.bin");
    cout << "file-back page for data1!" << endl;
    char *p3 = (char *)vm_map(filename_3, 0);

    cout << "swap-back page!" << endl;
    char *filename_4 = (char *)vm_map(nullptr, 0);
    strcpy(filename_4, "data3.bin");
    cout << "file-back page for data1!" << endl;
    char *p4 = (char *)vm_map(filename_4, 0);

    cout << "---------------------print p1   0---------------------!" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p1[i];
    }
    cout << endl;
    cout << "---------------------print p2   1---------------------!" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p2[i];
    }
    cout << endl;
    cout << "---------------------print p2   2---------------------!" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p4[i];
    }
    cout << endl;
    cout << "---------------------print p1   3---------------------!" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p1[i];
    }
    cout << endl;
    cout << "---------------------print p3   4---------------------!" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p3[i];
    }
    cout << endl;

    char *p5 = (char *)vm_map(filename, 0);
    vm_yield();
    cout << "---------------------print p2   5---------------------!" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p2[i];
    }
    cout << endl;

    for (unsigned int i=0; i<32; i++) {
        cout << p5[i];
    }

}