#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;
using std::string;

int main(){
    char *p0 = (char *) vm_map(nullptr, 0);
    char *p1 = (char *) vm_map(nullptr, 0);
    char *p2 = (char *) vm_map(nullptr, 0);

    char * file1 = p0;
    char * file2 = p0 + 20;
    char * file3 = p1 + 7;
    char * file4 = p1 + VM_PAGESIZE - 3;
    strcpy(file1, "data1.bin");
    strcpy(file2, "data2.bin");
    strcpy(file3, "data1.bin");
    strcpy(file4, "data1.bin");


    char *fp1 = (char *) vm_map(file1, 0);

    strcpy(fp1, "aaaaaa");


    char *fp2 = (char *) vm_map(file2, 1);
    char *fp3 = (char *) vm_map(file2, 1);
    // if (fork()) {
    //     vm_yield();
        char *p4 = (char *) vm_map(nullptr, 0);
        strcpy(p4, "data1.bin");
        char *fp5 = (char *) vm_map(p4, 0);
        for(unsigned int i = 0; i < 2; ++i){
            cout << "fp1: " << fp1[i] << endl;
            cout << "fp2: " << fp2[i] << endl;
            cout << "fp5: " << fp5[i] << endl;
        }
    // }    
    

    char *fp4 = (char *) vm_map(file4, 2);
    strcpy(fp4, "bbbbbbbb");


    for(unsigned int i=0;i<6;++i){
        cout << "fp1: " << fp1[i] << endl;
        cout << "fp2 write: "<< endl;
        fp2[i] = 'a';
        cout << "fp3: " << fp3[i] << endl;
        cout << "fp4: " << fp4[i] << endl;
    }
    cout<<"End"<<endl;
}