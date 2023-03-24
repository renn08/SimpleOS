// Test error block

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
    strcpy(filename, "");
    char * filename2 = (char *)vm_map(nullptr, 0);

    // cout << "filename: " << filename[0] << endl;
    // /* Write the name of the file that will be mapped */
    strcpy(filename2, "");
    cout << filename2 << endl;
    // /* Map a page from the specified file */
    char *p = (char *) vm_map (filename2, 1);
    for (int i = 0; i < 32; i++){
        cout<<p[i];
    }
    return 0;
}