#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

void parent()
{
    cout << "swap" << endl;
    char *filename = (char *)vm_map(nullptr, 0);
    strcpy(filename, "lampson83.txt");
    cout << "file for lampson83" << endl;
    char *p1 = (char *)vm_map(filename, 0);

    cout << "swap" << endl;
    char *filename2 = (char *)vm_map(nullptr, 0);
    strcpy(filename2, "data1.bin");
    cout << "file for data1" << endl;
    char *p2 = (char *)vm_map(filename2, 0);

    cout << "---parent read lampson83 1---" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout<<p1[i];
    }
    cout << endl;
    cout<<"parent vm_yield" << endl;
    vm_yield();

    cout << "---parent write lampson83 2---" << endl;
    for (unsigned int i=0; i<32; i++) {
        p1[i] = 'p';
    }
    cout << endl;
    cout<<"parent vm_yield" << endl;
    vm_yield();

    cout << "swap" << endl;
    char *filename3 = (char *)vm_map(nullptr, 0);
    strcpy(filename3, "data2.bin");
    cout << "file for data2" << endl;
    char *p3 = (char *)vm_map(filename3, 0);

    cout << "swap" << endl;
    char *filename4 = (char *)vm_map(nullptr, 0);
    strcpy(filename4, "data3.bin");
    cout << "file for data1" << endl;
    char *p4 = (char *)vm_map(filename4, 0);

    cout << "---parent print lampson83 3---" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p1[i];
    }
    cout << endl;
    cout << "---parent print data1 4---" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p2[i];
    }
    cout << endl;
    cout << "---parent print data3 5---" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p4[i];
    }
    cout << endl;
    cout << "---parent print lampson83 6---" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p1[i];
    }
    cout << endl;
    cout << "---parent print data2 7---" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p3[i];
    }
    cout << endl;

    char *p5 = (char *)vm_map(filename, 0);
    char *filename5 = (char *)vm_map(nullptr, 0);
    cout << "---parent print data1 8---" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p2[i];
    }
    cout << endl;

    cout << filename5 << endl;
    for (unsigned int i=0; i<32; i++) {
        cout << p5[i];
    }

}


void parent2(){

    cout << "swap" << endl;
    char *filename = (char *)vm_map(nullptr, 0);
    strcpy(filename, "lampson83.txt");
    cout << "file for lampson83" << endl;
    char *p1 = (char *)vm_map(filename, 0);

    cout << "swap" << endl;
    char *filename2 = (char *)vm_map(nullptr, 0);
    strcpy(filename2, "data1.bin");
    cout << "file for data1" << endl;
    char *p2 = (char *)vm_map(filename2, 0); 
    cout << "---parent2 read lampson83 1---" << endl;
    for (unsigned int i=0; i<32; i++) {
        cout<<p1[i];
    }
    cout << endl;
    cout<<"parent2 vm_yield" << endl;
    vm_yield();

    cout << "---parent2 write lampson83  2---" << endl;
    for (unsigned int i=0; i<32; i++) {
        p1[i] = 'c';
    }

    for (unsigned int i=0; i<32; i++) {
        cout << p1[i];
    }
    cout << endl;

    for (unsigned int i=0; i<32; i++) {
        cout << p2[i];
    }
    cout << endl;
}

int main() {
    if (fork()) {
        cout << "parent()" << endl;
        parent();
        cout << "parent() next line" << endl;

    } else {
        cout << "parent2()" << endl;
        parent2();
        cout << "parent2() next line" << endl;
    }
}