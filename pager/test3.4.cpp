// Test access an evicted file-backed block

#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

int main()
{
    char * filename1 = (char *)vm_map(nullptr, 0);
    strcpy(filename1, "data1.bin");
    char *p1 = (char *) vm_map (filename1, 0);
    for (unsigned int i=0; i<32; i++) {
        cout << p1[i];
    }
    cout << endl;
    
    char * filename2 = (char *)vm_map(nullptr, 0);
    strcpy(filename2, "data2.bin");
    char *p2 = (char *) vm_map (filename2, 0);
    for (unsigned int i=0; i<32; i++) {
        cout << p2[i];
    }
    cout << endl;

    char * filename3 = (char *)vm_map(nullptr, 0);
    strcpy(filename3, "data3.bin");
    char *p3 = (char *) vm_map (filename3, 0);
    for (unsigned int i=0; i<32; i++) {
        cout << p3[i];
    }
    cout << endl;
    
    char *p4 = (char *) vm_map (filename1, 0);
    for (unsigned int i=0; i<32; i++) {
        cout << p4[i];
    }
    cout << endl;
    return 0;
}