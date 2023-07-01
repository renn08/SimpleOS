#include <iostream>
#include <cstring>
#include <unistd.h>
#include "vm_app.h"

using std::cout;
using std::endl;
using std::string;
int main()
{
    cout << 1;
    /* Allocate swap-backed page from the arena */
    char *filename = (char *) vm_map(nullptr, 0);

    /* Write the name of the file that will be mapped */
    strcpy(filename, "data1.bin");
    /* Allocate swap-backed page from the arena */
    char * p[8];
    for (unsigned int i = 0; i < 8; i++) {
        p[i] = (char *) vm_map(nullptr, 1);
        strcpy(p[i], "  swap backed");
        p[i][0] = (char) i;
    }

    for (unsigned int i = 0; i < 8; i++) {
        cout << p[i][0] << endl;
    }

    /* Map a page from the specified file */
    char *f = (char *) vm_map (filename, 0);
    

    /* Print the first part of the paper */
    for (unsigned int i=0; i<10; i++) {

	    cout << f[i];

    }
}