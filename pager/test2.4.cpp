// Virtual page with same (filename, block)
// Test clock queue eviction

#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;

int main()
{
    /* Allocate swap-backed page from the arena */
    for (int i = 0; i < 9; i++) {
        char * filename = (char *)vm_map(nullptr, 0);
        // /* Write the name of the file that will be mapped */
        strcpy(filename, "lampson83.txt");
        cout << filename << endl;
        // /* Map a page from the specified file */
        char *p = (char *) vm_map (filename, 0);
        // /* Print the first part of the paper */
        for (unsigned int j=0; j<32; j++) {
            cout << p[j];
        }
        cout << endl;
    }
    return 0;
}