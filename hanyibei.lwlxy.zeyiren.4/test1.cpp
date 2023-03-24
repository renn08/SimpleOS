// test read existing fs and delete

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "fs_client.h"
using std::cout;

/*
 * test for initial, together with fs_image
 * under same user
 * /dir         /file
 * /other       /dir    /file
 * /file
 */

int main(int argc, char *argv[]) {
    char *server;
    int server_port;

    int status;
    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }

    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);

    status = fs_delete("user1", "/dir/file");
    assert(!status);
    status = fs_delete("user1", "/dir");
    assert(!status);
    status = fs_delete("user1", "/new/dir/file");
    assert(!status);
    status = fs_delete("user1", "/new/dir");
    assert(!status);
    status = fs_delete("user1", "/new");
    assert(!status);
    status = fs_delete("user1", "/file");
    assert(!status);
}