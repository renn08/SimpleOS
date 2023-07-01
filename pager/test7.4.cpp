#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

int main()
{
    /* Allocate swap-backed page from the arena */
    char * filename = (char *)vm_map(nullptr, 0);

    // cout << "filename: " << filename[0] << endl;
    // /* Write the name of the file that will be mapped */
    strcpy(filename+200, "data1.bin");
    cout << filename+200 << endl;
    // /* Map a page from the specified file */
    char *p = (char *) vm_map (filename+200, 0);
    // /* Print the first part of the paper */
    for (unsigned int i=0; i<32; i++) {
        cout << p[i];
    }
    cout << endl;
    filename = (char *)vm_map(nullptr, 0);
    char * filename1 = (char *)vm_map(nullptr, 0);
    strcpy(filename+65530, "data1.");
    cout << "????????????" <<endl;
    strcpy(filename1, "bin");
    // /* Map a page from the specified file */
    p = (char *) vm_map (filename + 65530, 0);
    // /* Print the first part of the paper */
    for (unsigned int i=0; i<32; i++) {
        cout << p[i];
    }
    cout << endl;
    return 0;
}