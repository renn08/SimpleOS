#include <unistd.h>
#include "vm_app.h"
#include <cassert>
#include <vector>
#include <iostream>
#include <cstring>


using std::cout;
using std::endl;

int main() {
    char *page_0 = (char *) vm_map(nullptr, 0);

    char *page_1 = (char *) vm_map(nullptr, 0);

    char *filename = page_0 + VM_PAGESIZE - 2;

    strcpy(filename, "data1.bin");

    strcpy(page_1, "data2.bin");

    /* Map a page from the specified file */
    std::vector<char *> page_map;

    for (int i = 0; i < 16; ++i) {
        page_map.push_back((char *) vm_map (filename, i));
    }

    char* paper_0 = page_map[0];

    if (fork()) {
        cout << "parent 1" << endl;
        paper_0[rand() % 256 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'A' + rand() % 26;
        if (fork()) {
            cout << "parent 2" << endl;
            paper_0[rand() % 256 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'A' + rand() % 26;
            if (fork()) {
                cout << "parent 3" << endl;
                paper_0[rand() % 256 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'A' + rand() % 26;
                if (fork()) {
                    cout << "parent 4" << endl;
                    paper_0[rand() % 256 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'A' + rand() % 26;
                    
                }
                else {
                    cout << "child 4" << endl;
                    paper_0[rand() % 256 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'A' + rand() % 26;
                }
            }
            else {
                cout << "child 3" << endl;
                paper_0[rand() % 256 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'A' + rand() % 26;
            }
            
        }
        else {
            cout << "child 2" << endl;
            paper_0[rand() % 256 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'A' + rand() % 26;
        }
    }
    else {
        cout << "child 1" << endl;
        paper_0[rand() % 256 * VM_PAGESIZE + rand() % VM_PAGESIZE] = 'A' + rand() % 26;
    }

    cout << filename << endl;
    cout << page_1 << endl;
    return 0;
}