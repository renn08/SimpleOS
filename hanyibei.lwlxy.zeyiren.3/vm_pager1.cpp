#include "vm_arena.h"
#include "vm_pager.h"
#include <iostream>
#include <vector>
#include <queue>
#include <deque>
#include <unordered_map>
#include <cassert>
#include <cstring>
#include <string>

using std::cout; 
using std::endl;
using std::vector;
using std::unordered_map;
using std::queue;
using std::deque;
using std::string;

bool DEBUG = false;
const unsigned int vpn_max = VM_ARENA_SIZE/VM_PAGESIZE;

// total number of swap blocks and physical memory page
unsigned int swap_blocks_num;
unsigned int memory_pages_num;
unsigned int swap_pages_num;


// status of swap blocks and memory pages, True is hold by at least one process.
vector<bool> swap_blocks_status;
vector<bool> physical_pages_status;
pid_t curr_id;

// The extra information of virtual page
struct Header {
    bool valid = false;
    bool is_file_backed;
    string filename = "";
    int block = -1;
};

// The information of process
struct Process {
    pid_t pid;
    // unsigned int vp_count = 0;
    unsigned int max_vp = 0;
    // Header header[VM_ARENA_SIZE/VM_PAGESIZE]; 
    vector<Header*> page_header;
    page_table_t page_table;
};

//Information of virtual page
struct PPage_entry {
    // The relevant virtual page
    vector<pid_t> process_id;
    vector<int> vp_index;
    unsigned int ppn;
    bool resident = false;
    bool dirty = false;
    bool referenced = false;
    // string filename = "";
    // bool is_file_backed;
    // int block = -1;
    Header header;
};

unordered_map<pid_t, Process*> pid_map; // map of process
unordered_map<unsigned int, PPage_entry> ppn_map; // map of physical page
deque<PPage_entry*> clock_queue; // all in-used physical pages

// check in arena
bool in_arena(const char* filename) {
    unsigned int vp = (uintptr_t)((uintptr_t)filename - (uintptr_t)VM_ARENA_BASEADDR) >> 16;
    if (vp>=vpn_max) return false;
    if (vp >= pid_map[curr_id]->max_vp) return false;
    if (!pid_map[curr_id]->page_header[vp]->valid) return false;
    return true;
}

// Find the true 
int find_free_swap_block() {
    int i = 0;
    for (bool it:swap_blocks_status) {
        // find the false one, empty one
        if (!it) {
            return i;
        }
        i++;
    }
    // is fully occupied
    return -1;
}

// for debug purpose
void see_clock_queue() {
    for (auto it = clock_queue.begin(); it != clock_queue.end(); ++it) {
        cout << "ppn: " << (*it)->ppn << "r" << (*it)->resident << "d" << (*it)->dirty << "r" << (*it)->referenced << endl;
    }    
}

void see_pysmem() {
    cout << "===================Show phymsm" << endl;
    for (int j = 0; j < 4; j++) {
        cout << "phymsm " << j << endl;
        for (int i = 0; i < 64; i++) {
            cout << *((char*)vm_physmem + (j << 16) + i);
        }
        cout << endl;
    }
}

// set the rbit and wbit and ppage of the virtual page that map to that physical page ppe, if these input params are smaller than 0, then remain the same
void set_vp_from_ppage_entry(PPage_entry* ppe, int r_bit, int w_bit, int ppage) {
    // assert(!ppe->process_id.empty());
    // assert(!ppe->vp_index.empty());
    int temp_i = 0;
    for (auto it : ppe->process_id) {
        if (r_bit >= 0) pid_map[it]->page_table.ptes[ppe->vp_index[temp_i]].read_enable = (unsigned int)r_bit;
        if (w_bit >= 0) pid_map[it]->page_table.ptes[ppe->vp_index[temp_i]].write_enable = (unsigned int)w_bit;
        if (ppage >= 0) pid_map[it]->page_table.ptes[ppe->vp_index[temp_i]].
        ppage = (unsigned int)ppage;
        temp_i++;
    }
}

// push the current page into clock queue
void clock_queue_push(PPage_entry *p_entry, string filename, int block, int vp) {
    if (DEBUG) cout << "filename: " << "block: " << block << endl;
    p_entry->process_id.clear();
    p_entry->process_id.push_back(curr_id);
    p_entry->vp_index.clear();
    p_entry->vp_index.push_back(vp);
    // update all the virtual page in different processes 's header the same as the updated one
    if (pid_map[curr_id]->page_header[vp]->is_file_backed) { 
        if (DEBUG) cout << "file_backed\n";
        for (auto [pid, process]:pid_map) {
            int i = 0;
            for (Header* header: process->page_header) {
                if (DEBUG) cout << i << " filename: "  << "block: " << header->block << endl;
                if (process->page_table.ptes[i].ppage == 0 &&
                    header-> is_file_backed &&
                    header->filename == filename &&
                    block == header-> block) {
                        if (DEBUG) cout << "find!!" << endl;
                        p_entry->process_id.push_back(pid);
                        p_entry->vp_index.push_back(i);
                        process->page_table.ptes[i].ppage = p_entry->ppn;
                        process->page_table.ptes[i].read_enable = pid_map[curr_id]->page_table.ptes[vp].read_enable;
                        process->page_table.ptes[i].write_enable = pid_map[curr_id]->page_table.ptes[vp].write_enable;
                    }
                if (DEBUG) cout << i << endl;
                i++;
            }
        }
    }

    clock_queue.push_back(p_entry);
    if (DEBUG) cout << "push finished" << endl;
}


// evict the first not referenced page in clock queue
unsigned int evict_clock_queue() {
    while(clock_queue.front()->referenced) {
        PPage_entry* temp = clock_queue.front();
        clock_queue.pop_front();
        temp->referenced = false;
        // to make sure to trap os into dealing with setting reference bit, set it as r0w0
        set_vp_from_ppage_entry(temp, 0, 0, -1); // if referenced is false, then read and write must be 0 and 0
        clock_queue.push_back(temp);
    }
    // until find a unreferenced page
    PPage_entry* temp = clock_queue.front();
    if (DEBUG) cout << temp->ppn << endl;
    clock_queue.pop_front();
    temp->resident = false;

    // Ppage with no mapped virtual page
    // All related processes end
    // Swap back page maybe dirty when the processes related to that page are all ended
    // Init dirty for later usage
    if (temp->process_id.empty()) {
        temp->dirty = false;
        return temp->ppn;
    }
    // set all virtual page of this ppn as 0, evict virtual page
    set_vp_from_ppage_entry(temp, -1, -1, 0);
    
    // if is swap backed, if is dirty, write to disk
    // assert(!(temp->process_id).empty());
    // assert(!(temp->vp_index).empty());
    if (!temp->header.is_file_backed) {
        if (temp->dirty) {
            // write to swap back block
            // find the swap block that is free
            assert(find_free_swap_block() != -1);
            int new_block = find_free_swap_block();
            file_write(nullptr, new_block, (void*)((uintptr_t)vm_physmem + (temp->ppn << 16)));
            if (DEBUG) see_pysmem();
            if (DEBUG) cout << *((char*)vm_physmem + (temp->ppn << 16) + 4) <<endl;
            if (DEBUG) cout << "ppn: " << temp->ppn <<endl;
            if (DEBUG) { 
                cout << "block: " << new_block <<endl;
                cout << "Process: " << temp->process_id[0] << endl;
                cout << "Index: " << temp->vp_index[0] << endl;
            }
            swap_blocks_status[new_block] = true;
            pid_map[temp->process_id[0]]->page_header[temp->vp_index[0]]->block = new_block;
            temp->dirty = false;
            // set_vp_from_ppage_entry(temp, -1, 0, -1);
        }
    } else {
        if (temp->dirty) {
            // write to file back block
            // find the file block
            file_write(temp->header.filename.c_str(), temp->header.block, (void*)((uintptr_t)vm_physmem + (temp->ppn << 16)));
            if (DEBUG) see_pysmem();
            temp->dirty = false;
            // set_vp_from_ppage_entry(temp, -1, 0, -1);
        }
        assert(!temp->dirty);
    }
    // memcpy(((char*)(vm_physmem) + (temp->ppn << 16)), ((char*)vm_physmem), VM_PAGESIZE);
    memset(((char*)vm_physmem + (temp->ppn << 16)), '\0', sizeof(char)*VM_PAGESIZE);
    if (DEBUG) see_pysmem();
    return temp->ppn;
}

// find the first availabe physical page to current virtual page
int check_phy_mem_empty_slot() {
    int index = 0;
    for (bool status: physical_pages_status) {
        if (!status) {
            return index;
        }
        index++;
    }
    // if is all full
    return -1;
}

// return the first availabe physical page to vm_fault
unsigned int check_phy_mem() {
    // the physical page is not full, no victim needed
    if (clock_queue.size() != memory_pages_num - 1) {
        // notice physical mem is start from 1
        // memcpy(((char*)(vm_physmem) + ((clock_queue.size() + 1) << 16)), ((char*)vm_physmem), VM_PAGESIZE);
        if (DEBUG) see_pysmem();
        int phy_index = check_phy_mem_empty_slot();
        memset(((char*)vm_physmem + ((phy_index) << 16)), '\0', sizeof(char)*VM_PAGESIZE);
        assert(phy_index > 0);
        physical_pages_status[phy_index] = true;
        memset(((char*)vm_physmem + (phy_index << 16)), '\0', sizeof(char)*VM_PAGESIZE);
        if (DEBUG) cout << "physical: " << phy_index << endl;
        return (unsigned int)phy_index;
    } else {
        return evict_clock_queue();
    }
}

// check filename in arena and properly stored and return it
std::pair<bool, string> check_filename(const char* filename){
    if (DEBUG) cout << "Begin check filename\n";
    // check filename in arena
    if (!in_arena(filename)) return std::make_pair(false, "");

    // check file is readable
    unsigned int vp = (uintptr_t)((uintptr_t)filename - (uintptr_t)VM_ARENA_BASEADDR) >> 16;
    if (DEBUG) cout << "vp: " << vp << " address: " << (uintptr_t)filename << " " << (uintptr_t)VM_ARENA_BASEADDR <<endl;
    if (DEBUG) cout << "in_arena passed\n";
    // if (pid_map[curr_id]->page_table.ptes[vp].read_enable == 0 || pid_map[curr_id]->page_table.ptes[vp].ppage == 0) {
    //     if (DEBUG) cout << "go to check vm+fault\n";
    //     if (vm_fault((const void *)filename, 0) == -1) return std::make_pair(false, "");
    // }
    unsigned int offset = (uintptr_t)filename - (vp << 16);
    unsigned int ppn = pid_map[curr_id]->page_table.ptes[vp].ppage;
    string result = "";
    // char* filename_curr = (char*)result.c_str();
    // strcpy(filename_curr, filename);

    while (vp < vpn_max) {
        if (DEBUG) cout << "vp:" << vp << "vpn_max" << vpn_max << endl;
        // check valid
        if (DEBUG) cout << "start check new page for file name" << endl;
        if (vp >= pid_map[curr_id]->max_vp) return std::make_pair(false, "");
        if (DEBUG) cout << "readable passed\n";
        if (!pid_map[curr_id]->page_header[vp]->valid) return std::make_pair(false, "");
        if (DEBUG) cout << "readable passed\n";

        // check residence
        if (pid_map[curr_id]->page_table.ptes[vp].read_enable == 0 || ppn == 0) {
            if (DEBUG) cout << "go to check vm+fault\n";
            if (vm_fault((const void *)((uintptr_t)(vp << 16) + (uintptr_t)VM_ARENA_BASEADDR), 0) == -1) return std::make_pair(false, "");
        } 

        if (DEBUG) cout << "readable passed\n";

        // //update referenced
        // ppn_map[ppn].referenced = true;
        // pid_map[curr_id]->page_table.ptes[vp].read_enable = 1;
        // pid_map[curr_id]->page_table.ptes[vp].write_enable = (int) ppn_map[ppn].dirty;
        // // TODO: update clock queue
        // clock_queue_push(&ppn_map[ppn], "", pid_map[curr_id])

        // check '\0'
        unsigned int ppn_curr = pid_map[curr_id]->page_table.ptes[vp].ppage;
        if (DEBUG) cout << "--------offset:" << offset <<"\n";
        for (; offset < VM_PAGESIZE; offset++){
            // if (DEBUG) cout << (ppn_curr << 16) << " " << offset << "???me?\n";
            // if (DEBUG) cout << offset << endl;
            // if (DEBUG) cout << (ppn_curr << 16) + offset << endl;
            
            unsigned int p_address = (ppn_curr << 16) + offset;
            if (DEBUG) cout << "ppn_curr:"<< ppn_curr << endl;
            if (DEBUG) cout << "here" << ((char *)vm_physmem)[p_address] << endl;
            if (DEBUG) cout << "here1" << endl;
            if (DEBUG) cout << ((char *)vm_physmem)[p_address] << endl;
            result +=  ((char *)vm_physmem)[p_address];
            if (DEBUG) cout << "result:" << result << endl;
            if (((char *)vm_physmem)[p_address] == '\0')  return std::make_pair(true, result);
        }
        offset = 0;
        vp++;
        // filename_curr = (char*) ((uintptr_t)(vp << 16) + (uintptr_t)VM_ARENA_BASEADDR);
    }
    return std::make_pair(false, "");
}


/*
 * vm_init
 *
 * Called when the pager starts.  It should set up any internal data structures
 * needed by the pager.
 *
 * vm_init is passed the number of physical memory pages and the number
 * of blocks in the swap file.
 */
void vm_init(unsigned int memory_pages, unsigned int swap_blocks) {
    if (DEBUG) std::cout<< "init begin\n" << std::endl;
    if (DEBUG) std::cout<< "physical pages: " << memory_pages << std::endl;
    if (DEBUG) std::cout<< "swap blocks: " << swap_blocks << std::endl;
    // init global variables
    memory_pages_num = memory_pages;
    // make sure zero page is empty TODO:change to memset
    // for (int i = 0; i < (int)VM_PAGESIZE; i++) {
    //     ((char*)vm_physmem)[i] = '\0';
    // }
    memset((char*)vm_physmem, '\0', sizeof(char)*VM_PAGESIZE);
    swap_blocks_num = swap_blocks;
    swap_pages_num = 0;
    // initialize physical page map, ppn -> ppage_entry
    for (unsigned int i = 0; i < memory_pages; ++i) {
        ppn_map[i].ppn = i;
        // init zero page to resident = true
        if (i == 0) {
            ppn_map[i].resident = true;
            physical_pages_status.push_back(true);
        } else {
            // init physical page status
            physical_pages_status.push_back(false);
        }
    }
    // init swap block status
    for (unsigned int i = 0; i < swap_blocks; ++i) {
        swap_blocks_status.push_back(false);
    }
    // TODO: init curr_id
    if (DEBUG) std::cout<< "init end\n" << std::endl;
}

/*
 * vm_create
 * Called when a parent process (parent_pid) creates a new process (child_pid).
 * vm_create should cause the child's arena to have the same mappings and data
 * as the parent's arena.  If the parent process is not being managed by the
 * pager, vm_create should consider the arena to be empty.
 * Note that the new process is not run until it is switched to via vm_switch.
 * Returns 0 on success, -1 on failure.
 */
 /* Consider only empty parent */
int vm_create(pid_t parent_pid, pid_t child_pid) {
    pid_map[child_pid] = new Process;
    for (unsigned int i = 0; i < VM_ARENA_SIZE/VM_PAGESIZE; i++) {
        pid_map[child_pid]->page_table.ptes[i].ppage = 0;
        pid_map[child_pid]->page_table.ptes[i].read_enable = 0;
        pid_map[child_pid]->page_table.ptes[i].write_enable = 0;
    }
    // pid_map[child_pid]->pid = child_pid;
    return 0;
}

/*
 * vm_switch
 *
 * Called when the kernel is switching to a new process, with process
 * identifier "pid".
 */
void vm_switch(pid_t pid) {
    // pid invalid
    assert(pid_map.find(pid) != pid_map.end());
    //     throw error;
    // }
    curr_id = pid;
    page_table_base_register = &pid_map[pid]->page_table;
}

/*
 * vm_fault
 *
 * Called when current process has a fault at virtual address addr.  write_flag
 * is true if the access that caused the fault is a write.
 * Returns 0 on success, -1 on failure.
 */
int vm_fault(const void* addr, bool write_flag) {
    if (DEBUG) {
        cout << "begin of the vm_fault" << endl;
        see_clock_queue();
    }
    unsigned int vp = (uintptr_t)((uintptr_t)addr - (uintptr_t)VM_ARENA_BASEADDR) >> 16;
    if (DEBUG) cout << "vp " << vp << endl;
    if (DEBUG) cout << pid_map[curr_id] << pid_map[curr_id]->page_header[vp] << endl;
    string true_filename = pid_map[curr_id]->page_header[vp]->filename;
    // current ppn
    if (DEBUG) cout << "true filename" << endl;
    unsigned int ppn = pid_map[curr_id]->page_table.ptes[vp].ppage;
    if (DEBUG) cout << "read bit: " << pid_map[curr_id]->page_table.ptes[vp].read_enable << endl;
    // if (pid_map[curr_id]->page_table.ptes[vp].read_enable == 0) return false;
    // unsigned int offset = (uintptr_t)addr - ((uintptr_t)VM_ARENA_BASEADDR << 16);
    // return error if invalid arena
    if (vp > pid_map[curr_id]->max_vp) return -1;

    if (DEBUG) {
        cout << "vp: " << vp << endl;   
        // cout << "valid: " << pid_map[curr_id]->page_header[vp]->valid << endl;
        cout << "resident: " << ppn_map[ppn].resident << endl;
    }
    // if write fault
    if (write_flag) {

        // 00
        // ppn = 0
        if (!pid_map[curr_id]->page_header[vp]->valid && !ppn_map[ppn].resident) return -1;
        if (DEBUG) cout << "not me" << endl;

        // 10
        // ppn = 0
        if (!ppn_map[ppn].resident || ppn == 0) {
            // find the ppn to put in
            ppn = check_phy_mem();
            // file read from the mem
            Header* vp_header = pid_map[curr_id]->page_header[vp];
            if (vp_header->is_file_backed) {
                if (file_read(vp_header->filename.c_str(), vp_header->block, (void*)((uintptr_t)vm_physmem + (ppn << 16))) == -1) return -1;
                if (DEBUG) see_pysmem();
            } else {
                if (vp_header->block != -1) {
                    if (file_read(nullptr, vp_header->block, (void*)((uintptr_t)vm_physmem + (ppn << 16))) == -1) return -1;
                } else {
                    memset(((char*)vm_physmem + (ppn << 16)), '\0', sizeof(char)*VM_PAGESIZE);
                }
                    
                if (DEBUG) see_pysmem();
                if (DEBUG) cout << vp << " writein ppn: " << ppn << "block: " << vp_header->block << endl;
            }
            ppn_map[ppn].dirty = true;
            ppn_map[ppn].header = *vp_header;
            pid_map[curr_id]->page_table.ptes[vp].read_enable = 1;
            pid_map[curr_id]->page_table.ptes[vp].write_enable = 1;
            // Update the virtual page of this physical page (and process id)
            // ppn_map[ppn]->process_id.clear();
            // ppn_map[ppn]->process_id.push_back(curr_id);
            // ppn_map[ppn]->vp_index.clear();
            // ppn_map[ppn]->vp_index.push_back(vp);
            // clock_queue.push(ppn_map[ppn]);
            clock_queue_push(&ppn_map[ppn], true_filename, vp_header->block, vp);
            pid_map[curr_id]->page_table.ptes[vp].ppage = ppn;
            ppn_map[ppn].resident = true;
            ppn_map[ppn].referenced = true;
            // pid_map[curr_id]->page_table[vp].read_enable = 1;//TODO:? not sure set to what
        } else {
            // if (pid_map[curr_id]->page_header[vp]->is_file_backed) {
            //     ppn_map[ppn]->referenced = true;
            //     // if (file_write(pid_map[curr_id]->page_header[vp]->filename, pid_map[curr_id]->page_header[vp]->block, (void*)((uintptr_t)vm_physmem + (ppn << 16))) == -1) return -1;
            //     if (DEBUG) cout << *((char*)vm_physmem + (ppn << 16)) <<endl;
            //     if (DEBUG) cout << "ppn: " << ppn <<endl;
            // } else {
            // if (pid_map[curr_id]->page_table.ptes[vp].ppage == 0) {
            //     // if is a newly assigned page and write trigger vm fault
            //     ppn = check_phy_mem();
            //     // make a copy of the zero page
            //     // change the defualt resident page to true
            //     ppn_map[ppn].resident = true;
            //     pid_map[curr_id]->page_table.ptes[vp].ppage = ppn;
            //     // update clock queue, insert that valid p page
            //     pid_map[curr_id]->page_table.ptes[vp].read_enable = 1;
            //     pid_map[curr_id]->page_table.ptes[vp].write_enable = 1;
            //     clock_queue_push(&ppn_map[ppn], true_filename, 0, vp);
            // }

            // init of 
            ppn_map[ppn].referenced = true;
            set_vp_from_ppage_entry(&ppn_map[ppn], 1, 1, -1);
            // pid_map[curr_id]->page_table.ptes[vp].read_enable = 1;
            // pid_map[curr_id]->page_table.ptes[vp].write_enable = 1;
            ppn_map[ppn].dirty = true;
            ppn_map[ppn].header = *pid_map[curr_id]->page_header[vp];
            // }
        }
    } else { // if read fault
        if (DEBUG) cout <<vp << " " << pid_map[curr_id]->page_header.size() <<endl;
        if (!pid_map[curr_id]->page_header[vp]->valid && !ppn_map[ppn].resident) return -1;
        if (DEBUG) cout <<"not me" <<endl;

        //update referenced
        // if read from a zero page, should find the content from the file and treat as non-resident
        if (!ppn_map[ppn].resident || ppn == 0) {
            // 10
            if (DEBUG) {cout << "in not resident" << endl;}
            // update clock queue
            ppn = check_phy_mem();
            // if (file_read(pid_map[curr_id]->page_header[vp]->filename, pid_map[curr_id]->page_header[vp]->block, (void*)((uintptr_t)vm_physmem + (ppn << 16))) == -1) return -1;
            // file read from the mem
            Header* vp_header = pid_map[curr_id]->page_header[vp];
            if (vp_header->is_file_backed) {
                if (DEBUG) {
                    //cout << "filename: " << vp_header->filename << endl;
                    cout << "ppn: " << ppn << endl;
                    cout << "block: " << vp_header->block << endl;
                    for (int i= 0; i < 10; i++) {
                        cout << *((char*)vm_physmem + (ppn << 16) + i) <<endl;
                    }
                }
                if (file_read(vp_header->filename.c_str(), vp_header->block, (void*)((uintptr_t)vm_physmem + (ppn << 16))) == -1) return -1;
                if (DEBUG) see_pysmem();
            } else {
                if (DEBUG) cout << *((char*)vm_physmem + (ppn << 16)) <<endl;
                if (DEBUG) cout << "REadin ppn: " << ppn <<endl;
                if (file_read(nullptr, vp_header->block, (void*)((uintptr_t)vm_physmem + (ppn << 16))) == -1) return -1;
                if (DEBUG) see_pysmem();
                if (DEBUG) {
                    cout << vp << " writein ppn: " << ppn << "block:;;;;;;; " << vp_header->block << endl;
                    for (int i= 0; i < 10; i++) {
                        cout << *((char*)vm_physmem + (ppn << 16) + i) <<endl;
                    }
                }
            }
            
            // init of virtual page
            pid_map[curr_id]->page_table.ptes[vp].ppage = ppn;
            if (DEBUG) cout << "-----------vp: " << vp <<endl;
            ppn_map[ppn].referenced = true;
            ppn_map[ppn].resident = true;
            pid_map[curr_id]->page_table.ptes[vp].read_enable = 1;
            pid_map[curr_id]->page_table.ptes[vp].write_enable = 0;
            ppn_map[ppn].header = *pid_map[curr_id]->page_header[vp];
            clock_queue_push(&ppn_map[ppn], true_filename, vp_header->block, vp);
            // TODO: write_enable set to what not sure? not always?
        } else {
            // 11
            ppn_map[ppn].referenced = true;
            // set readenable to 1
            // read fault from check file because of the filename is stored in the swapfile
            // and restore the write bit the same as before evict.
            set_vp_from_ppage_entry(&ppn_map[ppn], 1, (int)ppn_map[ppn].dirty, -1);// for swap backed, there will only be one process has this virtual page which map to this physical page, for file backed, set all the process's page table virtual page that map to that physical page to r_bit = 1, since for file backed vritual page are treated as one virtual page.

        }
    }
    if (DEBUG) {
        see_clock_queue();
    }
    return 0;
}

/*
 * vm_destroy
 *
 * Called when current process exits.  This gives the pager a chance to
 * clean up any resources used by the process.
 */
void vm_destroy() {
    // ppage entry vector 
    int id = 0;
    if (DEBUG) std::cout<< "pid: " << curr_id << "\n";
    for (Header* it:pid_map[curr_id]->page_header) {
        unsigned int ppn = pid_map[curr_id]->page_table.ptes[id].ppage;
        PPage_entry* curr_ppage = &ppn_map[ppn];
        int size = curr_ppage->process_id.size() - 1;
        if (DEBUG) std::cout<< "size: " << size << "\n";
        for (int i = size; i >= 0; i --) {
            if (DEBUG) std::cout<< "delete ppage index " << i << "\n";
            if (curr_ppage->process_id[i] == curr_id) {
                curr_ppage->vp_index.erase(i + curr_ppage->vp_index.begin());
                curr_ppage->process_id.erase(i + curr_ppage->process_id.begin());
            }
        }
        if (DEBUG) std::cout<< "finish delete ppage index\n";
        if (!it->is_file_backed) {
            // reset swap block vector
            if (it->block != -1) 
                swap_blocks_status[it->block] = false;
            // ppn_map[ppn].dirty = 0;
            // remove the process owned swap ppage from clock queue TODO:
            for (auto it2 = clock_queue.begin(); it2 < clock_queue.end(); ++it2) {
                if ((*it2)->ppn == ppn) {
                    assert(ppn != 0);
                    clock_queue.erase(it2);
                    physical_pages_status[ppn] = false;
                }
            }
            // reduce swap block num
            swap_pages_num--;
        }
        id++;
    }
    vector<Header*>::iterator it = pid_map[curr_id]->page_header.begin();
    if (DEBUG) std::cout<< "page header size: " << pid_map[curr_id]->page_header.size() << "\n";
    while (it != pid_map[curr_id]->page_header.end()) {
        if (DEBUG) std::cout<< "delete page header " << "\n";
        if (DEBUG) std::cout<< &it << "\n";
        delete *it;
        it ++;
    }
    if (DEBUG) std::cout<< "finish delete page header\n";
    // pid map
    delete pid_map[curr_id];
    if (DEBUG) std::cout<< "finish delete process\n";
    pid_map.erase(curr_id);
    if (DEBUG) std::cout<< "???????\n";

    //TODO: totally finished delete ppn entry
}

/*
 * vm_map
 *
 * A request by the current process for the lowest invalid virtual page in
 * the process's arena to be declared valid.  On success, vm_map returns
 * the lowest address of the new virtual page.  vm_map returns nullptr if
 * the arena is full.
 *
 * If filename is nullptr, block is ignored, and the new virtual page is
 * backed by the swap file, is initialized to all zeroes (from the
 * application's perspective), and private (i.e., not shared with any other
 * virtual page).  In this case, vm_map returns nullptr if the swap file is
 * out of space.
 *
 * If filename is not nullptr, the new virtual page is backed by the specified
 * file at the specified block and is shared with other virtual pages that are
 * mapped to that file and block.  filename is a null-terminated C string and
 * must reside completely in the valid portion of the arena.  In this case,
 * vm_map returns nullptr if filename is not completely in the valid part of
 * the arena.
 * filename is specified relative to the pager's current working directory.
 */
void *vm_map(const char *filename, unsigned int block) {
    // if (DEBUG) cout << filename << endl;
    if (DEBUG) std::cout<< "map begin\n" << std::endl;
    if (pid_map[curr_id]->max_vp >= vpn_max) {
        return nullptr;
    }
    //return page address
    void* addr = (void*)((uintptr_t)VM_ARENA_BASEADDR + (pid_map[curr_id]->max_vp << 16));
    // swap backed pages
    if (filename == nullptr) {
        if (swap_blocks_num == swap_pages_num) {
            if (DEBUG) std::cout<< "swap block full\n";
            return nullptr;
        }
        swap_pages_num += 1;
        Header* new_page = new Header();
        if (DEBUG) std::cout<< "create new header\n";
        unsigned int curr_vp = pid_map[curr_id]->max_vp;
        if (DEBUG) std::cout<< "curr vp assigned\n";
        pid_map[curr_id]->page_header.push_back(new_page);
        pid_map[curr_id]->page_table.ptes[curr_vp].ppage = 0;
        pid_map[curr_id]->page_table.ptes[curr_vp].read_enable = 1; // TODO:
        pid_map[curr_id]->page_table.ptes[curr_vp].write_enable = 0;
        if (DEBUG) std::cout<< "page table assigned\n";
        pid_map[curr_id]->page_header[curr_vp]->is_file_backed = 0;
        pid_map[curr_id]->page_header[curr_vp]->valid = true;
        if (DEBUG) std::cout<< "page header assigned\n";
        // pid_map[curr_id]->vp_count += 1;
        pid_map[curr_id]->max_vp += 1;
    
        return addr;
    } else {
        // file backed pages
        // check filename validation
        bool check;
        string true_filename;
        std::tie(check, true_filename) = check_filename(filename);
        if (!check) {
            if (DEBUG) cout << "check_filename failed\n";
            return nullptr;
        }

        // input filename vp and offset calculate
        unsigned int vp = (uintptr_t)((uintptr_t)filename - (uintptr_t)VM_ARENA_BASEADDR) >> 16;
        // unsigned int offset = (uintptr_t)filename - (vp << 16);
        // char* true_filename = (char*)((uintptr_t)vm_physmem + (pid_map[curr_id]->page_table.ptes[vp].ppage << 16) + offset);
        if (DEBUG) cout << "vp: " << vp << "name given ppn: " << pid_map[curr_id]->page_table.ptes[vp].ppage << endl;
        unsigned int curr_vp = pid_map[curr_id]->max_vp;

        // Check if (filename, block) already in physical page
        for (auto [key, ppage]:ppn_map) { 
            if (DEBUG) cout << "Physical page not empty\n";
            // pid_t pid = ppage.process_id[0];
            // int index = ppage.vp_index[0];
            Header* vp_header = &ppage.header;
            unsigned int ppn = ppage.ppn;
            if (vp_header->is_file_backed) {
                if (DEBUG) {
                    cout << "physical page is filebacked\n";
                    // cout << "===============\n original filename: " << vp_header->filename << " new filename: " << true_filename << "\n new block: " << vp_header->block << "original block:" << block << "\n";
                    cout << ppage.ppn << endl;
                    // cout << "HERE!" << true_filename <<endl;
                }
                if ( vp_header->filename == true_filename && vp_header->block == (int)block) {
                    if (DEBUG) cout << "Already in physical page\n";
                    Header* new_header = new Header;
                    pid_map[curr_id]->page_header.push_back(new_header);
                    pid_map[curr_id]->max_vp += 1;
                    pid_map[curr_id]->page_header[curr_vp]->block = block;
                    pid_map[curr_id]->page_header[curr_vp]->filename = true_filename;
                    pid_map[curr_id]->page_header[curr_vp]->is_file_backed = 1;
                    pid_map[curr_id]->page_header[curr_vp]->valid = ppage.header.valid;
                    pid_map[curr_id]->page_table.ptes[curr_vp].ppage = ppn;

                    // check whether all process mapped to this is evicted
                    // maybe the ppage don't have any vpage mapped to it, so we need to cal r and w based on referenced and dirty
                    if (ppage.process_id.empty()) {
                        if (!ppage.referenced) {
                            pid_map[curr_id]->page_table.ptes[curr_vp].read_enable = 0;
                            pid_map[curr_id]->page_table.ptes[curr_vp].write_enable = 0;
                        } else {
                            pid_map[curr_id]->page_table.ptes[curr_vp].read_enable = 1;
                            pid_map[curr_id]->page_table.ptes[curr_vp].write_enable = ppage.dirty;
                        }
                    } else {
                        pid_t pid = ppage.process_id[0];
                        int index = ppage.vp_index[0];
                        pid_map[curr_id]->page_table.ptes[curr_vp].read_enable = pid_map[pid]->page_table.ptes[index].read_enable;
                        pid_map[curr_id]->page_table.ptes[curr_vp].write_enable = pid_map[pid]->page_table.ptes[index].write_enable;
                    }
                    if (DEBUG) {
                        cout << "referenced" << ppn_map[ppn].referenced << endl;
                        cout << "dirty" << ppn_map[ppn].dirty << endl;
                        cout << "read_enable" << pid_map[curr_id]->page_table.ptes[curr_vp].read_enable << endl;
                        cout << "write_enable" << pid_map[curr_id]->page_table.ptes[curr_vp].write_enable << endl;
                    }
                    ppn_map[ppn].process_id.push_back(curr_id);
                    ppn_map[ppn].vp_index.push_back(curr_vp);
                    return addr;
                }
            }
        }

        Header* new_page = new Header();
        if (DEBUG) {
            cout << "VP:" << curr_vp << endl; 
            cout << pid_map[curr_id]->page_table.ptes[curr_vp-1].write_enable << endl;
        }
        pid_map[curr_id]->page_header.push_back(new_page);
        
        // find the filename, insert into header
        pid_map[curr_id]->page_header[curr_vp]->block = block;
        pid_map[curr_id]->page_header[curr_vp]->filename = true_filename;

        pid_map[curr_id]->page_table.ptes[curr_vp].ppage = 0;
        pid_map[curr_id]->page_table.ptes[curr_vp].read_enable = 0;
        pid_map[curr_id]->page_table.ptes[curr_vp].write_enable = 0;
        pid_map[curr_id]->page_header[curr_vp]->is_file_backed = 1;
        pid_map[curr_id]->max_vp += 1;
        pid_map[curr_id]->page_header[curr_vp]->valid = true;
        return addr;
    }
    if (DEBUG) std::cout<< "map end\n" << std::endl;
}