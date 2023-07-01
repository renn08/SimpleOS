// test create too many dir

#include <iostream>
#include <cassert>
#include <cstdlib>
#include "fs_client.h"

using std::cout, std::string;

int main(int argc, char *argv[]) {
    char *server;
    int server_port;

    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }

    server = argv[1];
    server_port = atoi(argv[2]);

    fs_clientinit(server, server_port);
    int status = 0;

    for (unsigned i = 0; i < 125; i++) {
        string name = "/";
        name += std::to_string(i);
        status = fs_create("user1", (const char *)name.c_str(), 'f');
        assert(!status);
    }

    status = fs_delete("user1", "/78");
    assert(!status);

    status = fs_delete("user1", "/39");
    assert(!status);

    const char *writedata =
            "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";

    status = fs_writeblock("user1", "/124", 0, writedata);
    cout << "write status: " << status << "\n";
}