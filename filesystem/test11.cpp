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
    // char readdata[FS_BLOCKSIZE];
    if (argc != 3) {
        cout << "error: usage: " << argv[0] << " <server> <serverPort>\n";
        exit(1);
    }
    server = argv[1];
    server_port = atoi(argv[2]);
    fs_clientinit(server, server_port);

    fs_create("user1", "/1", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/1", i, writedata);
    fs_create("user1", "/2", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/2", i, writedata);
    fs_create("user1", "/3", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/3", i, writedata);
    fs_create("user1", "/4", 'f');
    for (int i = 0; i < 123; i++)
        fs_writeblock("user1", "/4", i, writedata);
    fs_create("user1", "/5", 'f');
    fs_create("user1", "/6", 'f');
    fs_create("user1", "/7", 'f');
    fs_create("user1", "/8", 'f');
    fs_create("user1", "/9", 'f');
    fs_create("user1", "/10", 'f');
    fs_create("user1", "/11", 'f');
    fs_create("user1", "/12", 'f');
    fs_create("user1", "/13", 'f');
    fs_create("user1", "/14", 'd');
    fs_create("user1", "/15", 'f');

    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/15", i, writedata);
    fs_create("user1", "/16", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/16", i, writedata);
    fs_create("user1", "/17", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/17", i, writedata);
    fs_create("user1", "/18", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/18", i, writedata);
    fs_create("user1", "/19", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/19", i, writedata);
    fs_create("user1", "/20", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/20", i, writedata);
    fs_create("user1", "/21", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/21", i, writedata);
    fs_create("user1", "/22", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/22", i, writedata);
    fs_create("user1", "/23", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/23", i, writedata);
    fs_create("user1", "/24", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/24", i, writedata);
    fs_create("user1", "/25", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/25", i, writedata);
    fs_create("user1", "/26", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/26", i, writedata);
    fs_create("user1", "/27", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/27", i, writedata);
    fs_create("user1", "/28", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/28", i, writedata);
    fs_create("user1", "/29", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/29", i, writedata);
    fs_create("user1", "/30", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/30", i, writedata);
    fs_create("user1", "/31", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/31", i, writedata);
    fs_create("user1", "/32", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/32", i, writedata);
    fs_create("user1", "/33", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/33", i, writedata);
    fs_create("user1", "/34", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/34", i, writedata);
    fs_create("user1", "/35", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/35", i, writedata);
    fs_create("user1", "/36", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/36", i, writedata);
    fs_create("user1", "/37", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/37", i, writedata);
    fs_create("user1", "/38", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/38", i, writedata);
    fs_create("user1", "/39", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/39", i, writedata);
    fs_create("user1", "/40", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/40", i, writedata);
    fs_create("user1", "/41", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/41", i, writedata);
    fs_create("user1", "/42", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/42", i, writedata);
    fs_create("user1", "/43", 'f');
    for (int i = 0; i < 124; i++)
        fs_writeblock("user1", "/43", i, writedata);
}