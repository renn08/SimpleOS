#include <arpa/inet.h>		// htons()
#include <stdio.h>		// printf(), perror()
#include <stdlib.h>		// atoi()
#include <sys/socket.h>		// socket(), bind(), listen(), accept(), send(), recv()
#include <unistd.h>		// close()
#include <sstream>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>

extern std::mutex cout_lock;


#include "fs_server.h"
// #include "helpers.h"		// make_server_sockaddr(), get_port_number()

bool DEBUG = false;

using std::stringstream, std::cout, std::endl, std::string, std::istringstream, std::vector, std::unordered_map, std::mutex, std:: thread;


vector<bool> block_in_use;
unordered_map<uint32_t, mutex> mutex_look_up_dict; // from block num to mutex
mutex disk_mutex;


// static const size_t MAX_MESSAGE_SIZE = 256; // TODO: check

void exit_error(string s) {
    if (DEBUG) cout << s << endl;
    perror(s.c_str());
    throw std::runtime_error(s);
}

int find_free_block(){
    disk_mutex.lock();
    for (unsigned int i = 0; i < FS_BLOCKSIZE; i++) {
        if (!block_in_use[i]) {
            block_in_use[i] = true; // TODO: check if all has been changed to true
            disk_mutex.unlock();
            return i;
        }
    }
    disk_mutex.unlock();
    return -1;
}

unsigned int find_block(string username, vector<string> filename) {
    fs_inode relative_root_inode;
    int prev_root_block = 0;
    int curr_root_block = 0;
    cout << "here?" << endl;
    cout << mutex_look_up_dict.size() << endl;
    mutex_look_up_dict[prev_root_block].lock();
    cout << "no!" << endl;
    if (filename.size() == 1) return 0;
    disk_readblock(0, &relative_root_inode);
    for (size_t i=0; i < filename.size() - 1; i++) {
        if (i < filename.size()-1 && relative_root_inode.type == 'f') {
            mutex_look_up_dict[prev_root_block].unlock();
            exit_error("middle path is not directory");
        }
        if (DEBUG) cout << "filename: " << filename[i+1] << endl;
        bool found = false;
        for (size_t j = 0; j < relative_root_inode.size; ++j) {
            // each block init a fs_direntry list
            fs_direntry sub_direntry_list[FS_DIRENTRIES];
            // read data from fs
            disk_readblock(relative_root_inode.blocks[j], &sub_direntry_list);
            for (size_t k = 0; k < FS_DIRENTRIES; ++k) {
                uint32_t temp_inode_block = sub_direntry_list[k].inode_block;
                // unused if inode_block is 0
                if (DEBUG) cout << "find block inode block: " << sub_direntry_list[k].inode_block << endl;;
                if (temp_inode_block == 0) { 
                    if (DEBUG) cout << "No direntry here " << k << endl;
                    continue;
                }
                //check filename
                if (DEBUG) cout <<"stored name: "<<sub_direntry_list[k].name<<endl;
                if (strcmp(sub_direntry_list[k].name, filename[i+1].c_str()) == 0) {
                    curr_root_block = temp_inode_block;
                    found = true;
                    if (DEBUG) cout << "found" << endl;
                    break;
                }
            } 
            if (found) break;
        }
        if (!found) {
            mutex_look_up_dict[prev_root_block].unlock();
            exit_error("cannot find the path/file name");
        }
        if (DEBUG) cout << "into lock" << endl;
        mutex_look_up_dict[curr_root_block].lock();
        mutex_look_up_dict[prev_root_block].unlock();
        prev_root_block = curr_root_block;
        if (i < filename.size()-2) {
            disk_readblock(prev_root_block, &relative_root_inode);
            // check owner
            if (username != (string)relative_root_inode.owner  && curr_root_block!=0) {
                mutex_look_up_dict[prev_root_block].unlock();
                exit_error("ownership error");
            }
        }
    }
    return prev_root_block;
}

// fs_readblock
// TODO: exit unlock
void fs_readblock(string username, vector<string> filename, unsigned int block, char* data_temp) {
    unsigned int read_block = find_block(username, filename);
    fs_inode read_inode;
    if (DEBUG) cout << "read_inode: " << read_block << endl;
    if (DEBUG) cout << "it's here" << endl;
    disk_readblock(read_block, &read_inode);
    if (DEBUG) cout << "not me" << endl;
    if (read_inode.type == 'd') {
        mutex_look_up_dict[read_block].unlock();
        exit_error("readblock file type error");
    }
    if (username != (string)read_inode.owner && read_block!=0) {
        mutex_look_up_dict[read_block].unlock();
        exit_error("readblock ownership error");
    }
    if (read_inode.size <= block) {
        mutex_look_up_dict[read_block].unlock();
        exit_error("readblock block number error");
    }
    // char data_temp[FS_BLOCKSIZE];
    if (DEBUG) cout << "hello2" << endl;
    if (DEBUG) cout << read_inode.blocks[block] << endl;
    disk_readblock(read_inode.blocks[block], data_temp);
    if (DEBUG) cout << "it' not me 2" << endl;
    mutex_look_up_dict[read_block].unlock();
}

void fs_writeblock(string username, vector<string> filename, unsigned int block, char* data) {
    unsigned int write_block = find_block(username, filename);
    fs_inode write_inode;
    disk_readblock(write_block, &write_inode);
    if (write_inode.type == 'd') {
        mutex_look_up_dict[write_block].unlock();
        exit_error("writeblock file type error");
    }
    if (username != (string)write_inode.owner && write_block!=0) {
        mutex_look_up_dict[write_block].unlock();
        exit_error("writeblock ownership error");
    }
    if (write_inode.size + 1 <= block) {
        mutex_look_up_dict[write_block].unlock();
        exit_error("writeblock block number error");
    }
    int settle_block;
    if (block == write_inode.size) {
        // find empty block in file
        if (write_inode.size >= FS_MAXFILEBLOCKS) {
            mutex_look_up_dict[write_block].unlock();
            exit_error("writeblock block full");
        }
        // find empty block on disk
        settle_block = find_free_block();
        if (settle_block == -1) {
            mutex_look_up_dict[write_block].unlock();
            exit_error("writeblock disk full");
        }
        int entry = write_inode.size++;
        write_inode.blocks[entry] = settle_block;
        disk_writeblock(settle_block, data);
        disk_writeblock(write_block, &write_inode);
    } else {
        settle_block = write_inode.blocks[block];
        disk_writeblock(settle_block, data);
    }
    // mutex_look_up_dict[settle_block].lock();
    
    mutex_look_up_dict[write_block].unlock();
    // mutex_look_up_dict[settle_block].unlock();
}

void fs_createblock(string username, vector<string> filename, string new_name, char type) {
    unsigned int file_block = find_block(username, filename);
    fs_inode parent_inode;
    disk_readblock(file_block, &parent_inode);
    if (parent_inode.type == 'f') {
        mutex_look_up_dict[file_block].unlock();
        exit_error("createblock file type error");
    }
    if (username != (string)parent_inode.owner && file_block!=0) {
        mutex_look_up_dict[file_block].unlock();
        exit_error("createblock ownership error");
    }
    // find empty block in file
    if (parent_inode.size >= FS_MAXFILEBLOCKS) {
        mutex_look_up_dict[file_block].unlock();
        exit_error("createblock parent full");
    }

    // free block on disk for fs_inode
    int inode_settle_block, dir_settle_block;
    inode_settle_block = find_free_block();
    if (inode_settle_block == -1) {
        mutex_look_up_dict[file_block].unlock();
        cout << block_in_use[3755] << endl;
        exit_error("createblock disk full");
    }
    fs_inode new_inode;
    new_inode.size=0;
    strcpy(new_inode.owner, username.c_str());
    new_inode.type = type;
    int resident_block = -1;
    fs_direntry new_list[FS_DIRENTRIES];
    int parent_block = -1;
    // cout <<"size:" << parent_inode.size<<endl;

    // check empty direntry block
    for (size_t i = 0; i < parent_inode.size; ++i) {
        fs_direntry sub_direntry_list[FS_DIRENTRIES];
        // read data from fs
        disk_readblock(parent_inode.blocks[i], &sub_direntry_list);
        for (size_t j = 0; j < FS_DIRENTRIES; ++j) {
            uint32_t temp_inode_block = sub_direntry_list[j].inode_block;
            if (DEBUG) cout <<"inode block: " << temp_inode_block << ", resident block: " << resident_block << endl;
            // unused if inode_block is 0, create here
            if (temp_inode_block == 0 && resident_block == -1) {
                if (DEBUG) cout <<"parent: " << i << ", entry: " << j << endl;
                resident_block = temp_inode_block;
                memcpy(new_list, sub_direntry_list, sizeof(sub_direntry_list));
                new_list[j].inode_block = inode_settle_block;
                strcpy(new_list[j].name, new_name.c_str());
                parent_block = i;
            }
            if (new_name == (string)sub_direntry_list[j].name) {
                mutex_look_up_dict[file_block].unlock();
                exit_error("Repeated name");
            }
        } 
    }
    // don't need new block for direntry
    if (resident_block != -1) {
        // mutex_look_up_dict[inode_settle_block].lock();
        // mutex_look_up_dict[file_block].unlock();
        disk_writeblock(inode_settle_block, &new_inode);
        // mutex_look_up_dict[parent_inode.blocks[parent_block]].lock();
        // mutex_look_up_dict[inode_settle_block].unlock();
        disk_writeblock(parent_inode.blocks[parent_block], &new_list);
        // mutex_look_up_dict[parent_inode.blocks[parent_block]].unlock();
        mutex_look_up_dict[file_block].unlock();
        return;
    }

    // need to create new
    if (parent_inode.size >= FS_MAXFILEBLOCKS) {
        mutex_look_up_dict[file_block].unlock();
        exit_error("createblock parent full2");
    }
    // free block on disk for the new fs_direntry
    dir_settle_block = find_free_block();
    if (dir_settle_block == -1) {
        disk_mutex.lock();
        block_in_use[inode_settle_block] = false;
        disk_mutex.unlock();
        mutex_look_up_dict[file_block].unlock();
        exit_error("createblock disk full2");
    }
    mutex_look_up_dict[inode_settle_block];
    parent_inode.blocks[parent_inode.size++] = dir_settle_block;
    fs_direntry new_entries[FS_DIRENTRIES];
    new_entries[0].inode_block = inode_settle_block;
    strcpy(new_entries[0].name, new_name.c_str());
    // init new direntry block
    for (unsigned int i = 1; i < FS_DIRENTRIES; i++) {
        new_entries[i].inode_block = 0;
    }
    
    // mutex_look_up_dict[inode_settle_block].lock();
    // mutex_look_up_dict[file_block].unlock();
    disk_writeblock(inode_settle_block, &new_inode);
    // mutex_look_up_dict[dir_settle_block].lock();
    // mutex_look_up_dict[inode_settle_block].unlock();
    disk_writeblock(dir_settle_block, &new_entries);
    // mutex_look_up_dict[file_block].lock();
    // mutex_look_up_dict[dir_settle_block].unlock();
    disk_writeblock(file_block, &parent_inode);
    mutex_look_up_dict[file_block].unlock();
    return;
    // TODO: If read an unused block, but the block is found by a create
}

void fs_deleteblock(string username, vector<string> filename, string new_name) {
    // 0 means it's root
    if (filename.size() == 0) exit_error("delete root");
    // parent block (dir)
    unsigned int this_block_parent = find_block(username, filename);
    // read the parent inode
    fs_inode this_inode_parent;
    disk_readblock(this_block_parent, &this_inode_parent);
    if (username != (string)this_inode_parent.owner && this_block_parent != 0) {
        mutex_look_up_dict[this_block_parent].unlock();
        exit_error("deleteblock parent ownership error");
    }
    // find in the parent blocks
    if (DEBUG) cout << "delete parent size: " << this_inode_parent.size << endl;
    for (size_t i = 0; i < this_inode_parent.size; ++i) {
        fs_direntry sub_direntry_list[FS_DIRENTRIES];
        // read data from fs
        disk_readblock(this_inode_parent.blocks[i], &sub_direntry_list);
        for (size_t j = 0; j < FS_DIRENTRIES; ++j) {
            uint32_t temp_inode_block = sub_direntry_list[j].inode_block;
            // unused if inode_block is 0
            if (temp_inode_block == 0) continue;
            char* file_name = sub_direntry_list[j].name;
            if (DEBUG) cout << "curr: " << file_name << ", target: " << new_name<<endl;
            if ((string)file_name != new_name) continue;
            mutex_look_up_dict[temp_inode_block].lock();
            fs_inode temp_fs_inode;
            disk_readblock(temp_inode_block, &temp_fs_inode);
            if (username != (string)temp_fs_inode.owner) {
                mutex_look_up_dict[temp_inode_block].unlock();
                mutex_look_up_dict[this_block_parent].unlock();
                exit_error("deleteblock ownership error");
            }
            if (temp_fs_inode.size != 0 && temp_fs_inode.type == 'd') {
                mutex_look_up_dict[temp_inode_block].unlock();
                mutex_look_up_dict[this_block_parent].unlock();
                exit_error("delete dir not empty");
            }
            if (DEBUG) cout << "locked\n";
            
            disk_mutex.lock();
            block_in_use[temp_inode_block] = false;
            cout << "delete in use" << temp_inode_block << block_in_use[temp_inode_block] << endl;
            disk_mutex.unlock();
            cout << "unlocked" << endl;
            // if is file, also set its data block
            if (temp_fs_inode.type == 'f') {
                for (size_t k = 0; k < temp_fs_inode.size; ++k) {
                    disk_mutex.lock();
                    block_in_use[temp_fs_inode.blocks[k]] = false;
                    disk_mutex.unlock();
                }
            }
            sub_direntry_list[j].inode_block = 0;
            // check if need to shrink
            bool shrink = true;
            for (size_t m = 0; m < FS_DIRENTRIES; ++m) {
                if (sub_direntry_list[m].inode_block != 0) {
                    shrink = false;
                    break;
                }
            }

            // When FS_DELETE frees a directory entry, it should leave other entries in place, except when it can shrink the directory by an entire disk block by updating only the directory inode (in which case it should remove the unused block from the directory inode's blocks array, then shift all the following values in the blocks array up by one).

            // write to the new_name block
            if (shrink) {
                disk_mutex.lock();
                block_in_use[this_inode_parent.blocks[i]] = false;
                disk_mutex.unlock();
                for (size_t j = i + 1; j < this_inode_parent.size; ++j) {
                    this_inode_parent.blocks[j - 1] = this_inode_parent.blocks[j];
                }
                this_inode_parent.size--;
                disk_writeblock(this_block_parent, &this_inode_parent);
            } 
            else {
                disk_writeblock(this_inode_parent.blocks[i], &sub_direntry_list);
            }
            // if not shrink, nothing changed in parent blocks, no need to write

            mutex_look_up_dict[temp_inode_block].unlock();
            mutex_look_up_dict.erase(temp_inode_block);
            mutex_look_up_dict[this_block_parent].unlock();
            return;
        } 
    }
    mutex_look_up_dict[this_block_parent].unlock();
    exit_error("Didn't find delete file");
    return;
}

/**
 * Receives a string message from the client and prints it to stdout.
 *
 * Parameters:
 * 		connectionfd: 	File descriptor for a socket connection
 * 				(e.g. the one returned by accept())
 * Returns:
 *		0 on success, -1 on failure.
 */
int handle_connection(int connectionfd) {
    if (DEBUG) cout << connectionfd << endl;

	printf("New connection %d\n", connectionfd);

	// Call recv() enough times to consume all the data the client sends.
	size_t recvd = 0;
	ssize_t rval = 0;
    const int MAX_INPUT_SIZE = FS_MAXPATHNAME + FS_MAXFILENAME 
        + 3 // three space
        + 3 // at most 512 block (of length 3)
        + 1 // NULL
        + 13; // length of command FS_WRITEBLOCK

    char msg[MAX_INPUT_SIZE + FS_BLOCKSIZE]; // should be this not MAX_MESSAGE_SIZE
	memset(msg, 0, sizeof(msg));

    size_t sent_len = 0;


    try {
        if (DEBUG) cout << "maxinputsize " << MAX_INPUT_SIZE  << endl;
        do {
            // Receive as many additional bytes as we can in one call to recv()
            // (while not exceeding MAX_MESSAGE_SIZE bytes in total).
            rval = recv(connectionfd, msg + recvd, 1, MSG_NOSIGNAL);
            if (rval == -1) {
                exit_error("Error reading stream message");
            }
            recvd += 1;
        } while (rval > 0 && msg[recvd - 1] != '\0' && recvd <= MAX_INPUT_SIZE);  // recv() returns 0 when client close
        if (DEBUG) cout << "rvd: " << recvd << endl;
        // exceed max size
        if (recvd == MAX_INPUT_SIZE + 1) exit_error("exceeding max size without NULL");
        if (DEBUG) cout << "request: " << msg << endl;
        if (DEBUG) {
            cout << (sizeof(msg) / sizeof(char)) << endl;
            for (size_t a = 0; a < (sizeof(msg) / sizeof(char)); ++a) {
                cout << a;
                cout << *(msg + a);
            }
            cout << endl;
            }

        if (DEBUG) cout << "hello" << (string)msg << endl;

        istringstream s((string)msg);
        
        if (DEBUG) cout << "before while 0" << endl;
        string operation, username, pathname;
        unsigned int len = 0;
        if (!(s >> operation >> username >> pathname)) exit_error("operation missing arg error");
        if (username.length() > FS_MAXUSERNAME || pathname.length() > FS_MAXPATHNAME) exit_error("operation arg too long");
        if (pathname[0] != '/' || pathname[pathname.length()-1] == '/') exit_error("pathname with wrong / ");
        len = operation.length() + username.length() + pathname.length();
        vector<string> filename;
        if (DEBUG) cout << "before while1" << endl;
        std::stringstream file(pathname);
        string file_temp;
        if (DEBUG) cout << "before while2" << endl;
        while (std::getline(file, file_temp, '/')) {
            if (file_temp.length() > FS_MAXFILENAME) exit_error("filename too long");
            if (file_temp.length() == 0 && filename.size() > 0) exit_error("empty middle filename");
            if (DEBUG) cout << "in while: " << file_temp << endl;
            filename.push_back(file_temp);
        }
        if (DEBUG) cout << "operation: " << operation << endl;

        if (operation == "FS_READBLOCK") {
            string block_string;
            if (!(s >> block_string)) exit_error("readblock missing block arg");
            unsigned int block = atoi(block_string.c_str());
            if (block > FS_MAXFILEBLOCKS) exit_error("readblock block arg larger than maxblock");
            len += block_string.length();
            if (msg[len+3] != '\0') exit_error("readblock no NULL");
            if (DEBUG) cout << "read\n";
            // TODO: read
            if (DEBUG) cout << "this is "<< msg << " " << ((string)msg).length() << endl;
            
            // string res_msg = (string) msg + fs_readblock(username, filename, block);
            // strcpy(msg, res_msg.c_str());
            size_t cmd_len = strlen(msg) + 1;
            char data_temp[FS_BLOCKSIZE];
            fs_readblock(username, filename, block, data_temp);
            memcpy(msg + cmd_len, data_temp, FS_BLOCKSIZE); // + 1 for null
            sent_len = cmd_len + FS_BLOCKSIZE;
        } 
        else if (operation == "FS_WRITEBLOCK") {
            string block_string;
            if (!(s >> block_string)) exit_error("writeblock missing block arg");
            len += block_string.length();
            int block = atoi(block_string.c_str());
            if (msg[len+3] != '\0') exit_error("writeblock no NULL");
            char data[FS_BLOCKSIZE];
            int data_rval=0, data_recvd=0;
            if (DEBUG) cout << "write" << endl;
            do {
                data_rval = recv(connectionfd, data + data_recvd, 1, MSG_WAITALL);
                // data_rval = recv(connectionfd, data + data_recvd, FS_BLOCKSIZE - data_recvd, MSG_NOSIGNAL);
                // if (DEBUG) {
                //     cout << "read here: ";
                //     for (int i = 0; i < 512; i++) cout<<data[i];
                //     cout<<endl;
                // }
                if (data_rval == -1) {
                    perror("Error reading stream message");
                    return -1;
                }
                data_recvd += data_rval;
                // if (DEBUG) cout << "write?? " << data_recvd << " " << data_rval << " " << data[data_recvd-1]<< "\n";
            } while (data_recvd < (int)FS_BLOCKSIZE && data_rval > 0);
            if (DEBUG) cout << "write" << block << " " << data << "\n";
            // TODO: write
            fs_writeblock(username, filename, block, data);
            sent_len = strlen(msg) + 1;
        } 
        else if (operation == "FS_CREATE") {
            string type_string;
            if (!(s >> type_string)) exit_error("createblock missing type arg");
            if (type_string.length() != 1 || (type_string[0] != 'f' && type_string[0] != 'd')) exit_error("createblock type schema error");
            len += 1;
            if (msg[len+3] != '\0') exit_error("createblock missing NULL");
            if (DEBUG) cout << "create\n";
            // TODO: create
            string new_filename = filename.back();
            filename.pop_back();
            fs_createblock(username, filename, new_filename, type_string[0]);
            sent_len = strlen(msg) + 1;
        } 
        else if (operation == "FS_DELETE") {
            if (DEBUG) cout << "delete\n";
            if (msg[len+2] != '\0') exit_error("deleteblock missing NULL");
            // TODO: delete
            string new_filename = filename.back();
            filename.pop_back(); // the path to its parent
            fs_deleteblock(username, filename, new_filename);
            sent_len = strlen(msg) + 1;
        } 
        else {
            exit_error("Operation error");
        }

        if (DEBUG) cout << "send\n";
        if (send(connectionfd, msg, sent_len, MSG_NOSIGNAL) == -1) {
            throw std::runtime_error("Error sending on stream socket");
        }
        if (DEBUG) cout << "before close" << endl;
    } catch (std::runtime_error &error) {
        if (DEBUG) cout << "catch error" << endl;
    }
    // (4) Close connection
    close(connectionfd);
    if (DEBUG)  cout << "after close " << endl;

	return 0;
}

/**
 * Endlessly runs a server that listens for connections and serves
 * them _synchronously_.
 *
 * Parameters:
 *		port: 		The port on which to listen for incoming connections.
 *		queue_size: 	Size of the listen() queue
 * Returns:
 *		-1 on failure, does not return on success.
 */
int run_server(int port, int queue_size) {
    if (DEBUG) cout<<"start server\n";

	// (1) Create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Error opening stream socket");
		return -1;
	}

	// (2) Set the "reuse port" socket option
	int yesval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yesval, sizeof(yesval)) == -1) {
		perror("Error setting socket options");
		return -1;
	}

	// (3) Create a sockaddr_in struct for the proper port and bind() to it.
	struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

	// (3b) Bind to the port.
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("Error binding stream socket");
		return -1;
	}

	// (3c) Detect which port was chosen.
    socklen_t length = sizeof(addr);
    if (getsockname(sockfd, (sockaddr *)&addr, &length) == -1) {
        perror("Error getting port of socket");
        return -1;
    }
    port = ntohs(addr.sin_port);

	// (4) Begin listening for incoming connections.
	listen(sockfd, queue_size);

    cout_lock.lock();
    cout << "\n@@@ port " << port << endl;
    cout_lock.unlock();

	// (5) Serve incoming connections one by one forever.
	while (true) {
		int connectionfd = accept(sockfd, 0, 0);
        if (DEBUG) cout << connectionfd << endl;
		if (connectionfd == -1) {
			perror("Error accepting connection");
			return -1;
		}
        if (DEBUG) cout << "????" << endl;
        handle_connection(connectionfd);
		// thread t(handle_connection, connectionfd);
        // t.detach();
	}
}

// init the list of free disk blocks by reading the existed fs
void read_fs(fs_inode &relative_root_inode) {
    // cout << mutex_look_up_dict.size() << endl;
    if (relative_root_inode.size == 0) return;
    // if relative_root_inode is file
    if (relative_root_inode.type == 'f') {
        for (size_t i = 0; i < relative_root_inode.size; ++i) {
            block_in_use[relative_root_inode.blocks[i]] = true;
        }
        return;
    }
    // dir inode
    for (size_t i = 0; i < relative_root_inode.size; ++i) {
        // each block init a fs_direntry list
        fs_direntry sub_direntry_list[FS_DIRENTRIES];
        uint32_t block = relative_root_inode.blocks[i];
        // update block_in_use
        block_in_use[block] = true;
        // read data from fs
        disk_readblock(block, &sub_direntry_list);
        for (size_t j = 0; j < FS_DIRENTRIES; ++j) {
            uint32_t temp_inode_block = sub_direntry_list[j].inode_block;
            // unused if inode_block is 0
            if (temp_inode_block == 0) continue;
            block_in_use[temp_inode_block] = true;
            fs_inode temp_fs_inode;
            disk_readblock(temp_inode_block, &temp_fs_inode);
            // if (temp_fs_inode.size == 0) continue;
            mutex_look_up_dict[temp_inode_block];
            cout << "here?" << endl;
            read_fs(temp_fs_inode);
        } 
    }
    // if (DEBUG) cout << "finish read fs\n";
    // return;
}


int main(int argc, const char **argv) {
	// Parse command line arguments
    if (DEBUG) cout << "start main\n";
    int port = 0;
	if (argc == 1) {
        port = 0; // bind will select for us
	} else if (argc == 2) {
        port = atoi(argv[1]);
    } else {
	    printf("Usage: ./server port_num\n");
		return 1;
    }

    mutex_look_up_dict[0];

    // init block_in_use
    block_in_use.resize(FS_BLOCKSIZE, false);
    // root block always in use
    block_in_use[0] = true;
    if (DEBUG) cout << "becore Read in root node\n";

    // read the root inode
    fs_inode root_inode;
    disk_readblock(0, &root_inode);
    if (DEBUG) cout << "Read in root node\n";

    // read the file sys
    read_fs(root_inode);

    for (auto begin = mutex_look_up_dict.begin(); begin != mutex_look_up_dict.end();
            begin++) {
        cout_lock.lock();
        cout << "Map:" << begin->first << "\n";
        cout_lock.unlock();
    }

	if (run_server(port, 30) == -1) {
		return 1;
	}

	return 0;
}