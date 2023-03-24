#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

// test start with non-empty arena

using namespace std;

int main(){
    char *filename = (char *) vm_map(nullptr, 0);
    char *filename1 = (char *) vm_map(nullptr, 0);
    char *filename2 = (char *) vm_map(nullptr, 0);
    strcpy(filename, "data1.bin");
    strcpy(filename1, "data2.bin");
    strcpy(filename2, "data3.bin");
    fork();
    char *filename3 = (char *) vm_map(nullptr, 0); 
    strcpy(filename3, "lampson83.txt");
    char *p = (char *) vm_map (filename2, 0);
    strcpy(filename2, "data2.bin");
    strcpy(filename1, "data1.bin");
    strcpy(filename, "data3.bin");

    char *p1 = (char *) vm_map (filename, 0);
    cout << p1[0] << endl;
    char *p2 = (char *) vm_map (filename2, 0);
    cout << p2[0] << endl;
    char *p3 = (char *) vm_map (filename1, 0);
    cout << p3[0] << endl;
}