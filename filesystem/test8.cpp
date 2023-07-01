// test write and read in existing block

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <thread>
#include "fs_client.h"

using std::cout, std::thread;

void th1() {
    const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";

    const char *writedata2 = "Wer  from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";

    char readdata[FS_BLOCKSIZE];
    int status;
    status = fs_create("user1", "/dir", 'd');
    assert(!status);

    status = fs_create("user1", "/dir/file", 'f');
    assert(!status);

    status = fs_writeblock("user1", "/dir/file", 0, writedata);
    assert(!status);

    status = fs_writeblock("user1", "/dir/file", 1, writedata);
    assert(!status);

    status = fs_writeblock("user1", "/dir/file", 0, writedata2);
    assert(!status);

    status = fs_readblock("user1", "/dir/file", 0, readdata);
    assert(!status);

    status = fs_delete("user1", "/dir/file");
    assert(!status);

    status = fs_delete("user1", "/dir");
    assert(!status);
}

void th2() {
    const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";

    const char *writedata2 = "Wer  from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and Happiness.";

    char readdata[FS_BLOCKSIZE];
    int status;

    status = fs_writeblock("user2", "/dir2/file", 0, writedata);
    assert(!status);

    status = fs_writeblock("user2", "/dir2/file", 1, writedata);
    assert(!status);

    status = fs_writeblock("user2", "/dir2/file", 0, writedata2);
    assert(!status);

    status = fs_readblock("user2", "/dir2/file", 0, readdata);
    assert(!status);
}

void th3() {
    const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and shitiness.";

    const char *writedata2 = "Wer  from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and shitiness.";

    char readdata[FS_BLOCKSIZE];
    int status;

    status = fs_readblock("user2", "/dir2/file", 0, readdata);
    assert(!status);

    status = fs_writeblock("user2", "/dir2/file", 0, writedata);
    assert(!status);

    status = fs_writeblock("user2", "/dir2/file", 1, writedata);
    assert(!status);

    status = fs_writeblock("user2", "/dir2/file", 0, writedata2);
    assert(!status);
}


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

    int status;
    status = fs_create("user2", "/dir2", 'd');
    assert(!status);

    status = fs_create("user2", "/dir2/file", 'f');
    assert(!status);

    const char *writedata = "We hold these truths to be self-evident, that all men are created equal, that they are endowed by their Creator with certain unalienable Rights, that among these are Life, Liberty and the pursuit of Happiness. -- That to secure these rights, Governments are instituted among Men, deriving their just powers from the consent of the governed, -- That whenever any Form of Government becomes destructive of these ends, it is the Right of the People to alter or to abolish it, and to institute new Government, laying its foundation on such principles and organizing its powers in such form, as to them shall seem most likely to effect their Safety and daddyness.";

    status = fs_writeblock("user2", "/dir2/file", 0, writedata);
    assert(!status);

    thread t1(th1);
    thread t2(th2);
    thread t3(th3);
    thread t4(th3);
    thread t5(th3);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    status = fs_delete("user2", "/dir2/file");
    assert(!status);

    status = fs_delete("user2", "/dir2");
    assert(!status);
}