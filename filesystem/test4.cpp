// test error handling

#include <iostream>
#include <cassert>
#include <cstdlib>
#include "fs_client.h"

using std::cout;

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

    // try delete root
    fs_delete("user1", "/");

    // invalid path for create
    fs_create("user1", "/hello/hello/hello/hello", 'd');

    

    // test delete non-empty dir
    fs_create("user1", "/dir", 'd');
    // test create duplicate dir name
    fs_create("user1", "/dir", 'd');
    fs_create("user1", "/dir", 'd');

    // extra space
    fs_create("user1 ", "/dir1", 'd');
   

    fs_create("user1", "/dir/file", 'f');

    fs_create("", "/dir/file1", 'f');

    fs_create("user1", "", 'f');

    

    fs_writeblock("user1", "/dir/file ", 0, writedata);

    fs_writeblock("user1", "/dir/file", 0001, writedata);
    // test create duplicate file name
    fs_create("user1", "/dir/file", 'f');
    fs_create("user1", "/dir/file", 'f');
    // test too long path name, username
    fs_create("user1", "/dir/filehhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhgggggggggggggggggggggggggggggggggggggggghhhhhhhhhhhhhhhhhhhhhhhh", 'f');
    fs_create("user111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111", "/dir/file", 'f');
    // test wrong type
    fs_create("user1", "/dir/file", 'a');
    fs_writeblock("user1", "/dir/file", 0, writedata);

    // invalid path for read
    fs_readblock("user1", "/dir/hello", 1, readdata);

    // invalid path for write
    fs_writeblock("user1", "/dir/hello", 1, writedata);

    // invalid path for delete
    fs_delete("user1", "/dir/hello");

    // invalid path for delete
    fs_delete("user1", "/hello/helllllo");

    // read a dir instead of a file
    fs_readblock("user1", "/dir", 0, readdata);

    // write a dir instead of a file
    fs_writeblock("user1", "/dir", 0, writedata);

    // create with parent path as a file
    fs_create("user1", "/dir/file/file", 'f');
    fs_create("user1", "/dir/file/file", 'd');


    fs_delete("user1", "/dir");

    // test read non-existing block
    fs_readblock("user1", "/dir/file", 1, readdata);

    // test write block error
    fs_writeblock("user1", "/dir/file", 3, readdata);

    // different user -- create
    fs_create("user2", "/dir/file2", 'd');

    // different user -- delete
    fs_delete("user2", "/dir/file");

    // different user -- read
    fs_readblock("user2", "/dir/file", 0, readdata);

    // different user -- write
    fs_writeblock("user2", "/dir/file", 0, writedata);

}