#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include "fs_client.h"

using std::cout, std::endl;
using std::string;

/*
 * test when fs is full
 */

int main(int argc, char *argv[]) {
    char *server;
    int server_port;

    const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";
    char readdata[FS_BLOCKSIZE];
    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);
    fs_clientinit(server, server_port);

    fs_writeblock("user1", "/4", 88, writedata);
    fs_writeblock("user1", "/4", 123, writedata);
    fs_readblock("user1", "/1", 0, readdata);
    fs_create("user1", "/dir", 'd');
    fs_delete("user1", "/5");
    cout << "hi" << endl;
    fs_create("user1", "/14/test", 'd');
    cout << "write" << endl;
    fs_writeblock("user1", "/4", 123, writedata);
    cout << "hi" << endl;
    fs_delete("user1", "/6");
    fs_create("user1", "/dir/file", 'f');
    cout << "hi" << endl;
    fs_create("user1", "/dir", 'd');
    fs_create("user1", "/dir/file", 'f');
}