#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using namespace std;

int main(){
    /* Allocate swap-backed page from the arena */
    char* filename1 = (char *) vm_map(nullptr, 0);
    char* filename2 = (char *) vm_map(nullptr, 0);
    char* filename3 = (char *) vm_map(nullptr, 0);
    char* filename4 = (char *) vm_map(nullptr, 0);
    char* filename5 = (char *) vm_map(nullptr, 0);
    char* filename6 = (char *) vm_map(nullptr, 0);
    filename6[0] = 'a';
    if (!fork()){
        filename3[0] = 'c';
    }
    cout << filename1[0] << endl;
    cout << filename2[0] << endl;
    cout << filename4[0] << endl;
    cout << filename5[0] << endl;
    cout << filename6[0] << endl;
    cout << filename3[0] << endl;
    return 0;
}