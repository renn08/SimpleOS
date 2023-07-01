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


// the max virtual page number
const unsigned int vpn_max = VM_ARENA_SIZE / VM_PAGESIZE;

// total number of swap blocks
unsigned int swap_blocks_num;

// total number of physical memory page
unsigned int memory_pages_num;

// total number of swap pages at that time
unsigned int swap_pages_num;

// status of swap blocks and memory pages, True is hold by at least one process.
vector<bool> swap_blocks_status;
vector<bool> physical_pages_status;

// current process id
pid_t curr_id;

// The extra information of virtual page
struct Header {
    bool valid = false; // whether a page is reside in the arena
    bool is_file_backed;
    string filename = ""; // the filename of that vp if is file backed page
    int block = -1; // the blk of that vp if is file backed page
};

// The information of a process
struct Process {
    page_table_t page_table; // the virtual memory table
    vector<Header*> page_header; // the header of its owned vps
    pid_t pid;
    unsigned int max_vp = 0; // the current valid page number
};

// Information of a physical page
struct PPage_entry {
    vector<pid_t> process_id; // the process that has virtual pages mapped to it
    vector<int> vp_index; // the virtual page index (for Process.page_header) of the corresponding process in PPage_entry.process_id
    Header header;
    unsigned int ppn; // physical page number of that page
    bool resident = false;
    bool dirty = false;
    bool referenced = false;
};

unordered_map<pid_t, Process*> pid_map; // a map of all processes
unordered_map<unsigned int, PPage_entry> ppn_map; // a map of physical page from physical page number
deque<PPage_entry*> clock_queue; // all in-used physical pages, used as a queue

// function prototypes
bool in_arena(const char* filename);
int find_free_swap_block();
void see_clock_queue(); // DEBUG function
void see_pysmem(); // DEBUG function
void set_vp_from_ppage_entry(PPage_entry* ppe, int r_bit, int w_bit, int ppage);
void clock_queue_push(PPage_entry* ppe, string filename, int block, int vp);
unsigned int evict_clock_queue();
int check_phy_mem_empty_slot();
unsigned int check_phy_mem();
std::pair<bool, string> check_filename(const char* filename);


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
    // init global variables
    memory_pages_num = memory_pages;
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
    return 0;
}

/*
 * vm_switch
 *
 * Called when the kernel is switching to a new process, with process
 * identifier "pid".
 */
void vm_switch(pid_t pid) {
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
    unsigned int vp = (uintptr_t)((uintptr_t)addr - (uintptr_t)VM_ARENA_BASEADDR) >> 16;
    // return error if invalid arena
    if (vp >= pid_map[curr_id]->max_vp) return -1;
    string true_filename = pid_map[curr_id]->page_header[vp]->filename;
    // current ppn
    unsigned int ppn = pid_map[curr_id]->page_table.ptes[vp].ppage;

    // if write fault
    if (write_flag) {
        // 00
        if (!pid_map[curr_id]->page_header[vp]->valid && !ppn_map[ppn].resident) return -1;

        // 10
        // ppn = 0
        if (!ppn_map[ppn].resident || ppn == 0) {
            // find the ppn to put in
            ppn = check_phy_mem();
            
            // file read from the mem
            Header* vp_header = pid_map[curr_id]->page_header[vp];
            memset(((char*)vm_physmem + (ppn << 16)), '\0', sizeof(char)*VM_PAGESIZE);
            if (vp_header->is_file_backed) {
                if (file_read(vp_header->filename.c_str(), vp_header->block, (void*)((uintptr_t)vm_physmem + (ppn << 16))) == -1) return -1;
            } else {
                if (vp_header->block != -1) {
                    if (file_read(nullptr, vp_header->block, (void*)((uintptr_t)vm_physmem + (ppn << 16))) == -1) return -1;
                }
            }

            ppn_map[ppn].dirty = true;
            ppn_map[ppn].header = *vp_header;
            pid_map[curr_id]->page_table.ptes[vp].read_enable = 1;
            pid_map[curr_id]->page_table.ptes[vp].write_enable = 1;
            pid_map[curr_id]->page_table.ptes[vp].ppage = ppn;

            clock_queue_push(&ppn_map[ppn], true_filename, vp_header->block, vp);

            ppn_map[ppn].resident = true;
            ppn_map[ppn].referenced = true;
        } else {
            // 11
            ppn_map[ppn].referenced = true;
            set_vp_from_ppage_entry(&ppn_map[ppn], 1, 1, -1);
            ppn_map[ppn].dirty = true;
            ppn_map[ppn].header = *pid_map[curr_id]->page_header[vp];
        }
    } else { // if read fault
        // 00
        if (!pid_map[curr_id]->page_header[vp]->valid && !ppn_map[ppn].resident) return -1;

        // update referenced
        // if read from a zero page, should find the content from the file and treat as non-resident
        if (!ppn_map[ppn].resident || ppn == 0) {
            // 10
            // update clock queue
            ppn = check_phy_mem();
            // file read from the mem
            Header* vp_header = pid_map[curr_id]->page_header[vp];
            memset(((char*)vm_physmem + (ppn << 16)), '\0', sizeof(char)*VM_PAGESIZE);
            if (vp_header->is_file_backed) {
                if (file_read(vp_header->filename.c_str(), vp_header->block, (void*)((uintptr_t)vm_physmem + (ppn << 16))) == -1) return -1;
            } else {
                if (file_read(nullptr, vp_header->block, (void*)((uintptr_t)vm_physmem + (ppn << 16))) == -1) return -1;
            }
            
            // init of virtual page
            pid_map[curr_id]->page_table.ptes[vp].ppage = ppn;
            ppn_map[ppn].referenced = true;
            ppn_map[ppn].resident = true;
            pid_map[curr_id]->page_table.ptes[vp].read_enable = 1;
            pid_map[curr_id]->page_table.ptes[vp].write_enable = 0;
            ppn_map[ppn].header = *vp_header;
            clock_queue_push(&ppn_map[ppn], true_filename, vp_header->block, vp);
        } else {
            // 11
            ppn_map[ppn].referenced = true;
            // set readenable to 1
            // read fault from check file because of clock queue algorithm
            set_vp_from_ppage_entry(&ppn_map[ppn], 1, (int)ppn_map[ppn].dirty, -1);// for swap backed, there will only be one process has this virtual page which map to this physical page, for file backed, set all the process's page table virtual page that map to that physical page to r_bit = 1, since for file backed vritual page are treated as one virtual page.

        }
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
    for (Header* it:pid_map[curr_id]->page_header) {
        unsigned int ppn = pid_map[curr_id]->page_table.ptes[id].ppage;
        PPage_entry* curr_ppage = &ppn_map[ppn];
        int size = curr_ppage->process_id.size() - 1;
        for (int i = size; i >= 0; i --) {
            if (curr_ppage->process_id[i] == curr_id) {
                curr_ppage->vp_index.erase(i + curr_ppage->vp_index.begin());
                curr_ppage->process_id.erase(i + curr_ppage->process_id.begin());
            }
        }
        if (!it->is_file_backed) {
            // reset swap block vector
            if (it->block != -1) 
                swap_blocks_status[it->block] = false;
            ppn_map[ppn].dirty = 0;
            // remove the process owned swap ppage from clock queue 
            for (auto it2 = clock_queue.begin(); it2 < clock_queue.end(); ++it2) {
                if ((*it2)->ppn == ppn) {
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
    while (it != pid_map[curr_id]->page_header.end()) {
        delete *it;
        it ++;
    }
    // pid map
    delete pid_map[curr_id];
    pid_map.erase(curr_id);
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
    // check if arena is full
    if (pid_map[curr_id]->max_vp >= vpn_max) {
        return nullptr;
    }
    // return page address
    void* addr = (void*)((uintptr_t)VM_ARENA_BASEADDR + (pid_map[curr_id]->max_vp << 16));
    if (filename == nullptr) { // swap backed pages
        if (swap_blocks_num == swap_pages_num) {
            return nullptr;
        }
        swap_pages_num += 1;
        Header* new_page = new Header();
        unsigned int curr_vp = pid_map[curr_id]->max_vp;
        pid_map[curr_id]->page_header.push_back(new_page);
        pid_map[curr_id]->page_table.ptes[curr_vp].ppage = 0;
        pid_map[curr_id]->page_table.ptes[curr_vp].read_enable = 1; 
        pid_map[curr_id]->page_table.ptes[curr_vp].write_enable = 0;
        pid_map[curr_id]->page_header[curr_vp]->is_file_backed = 0;
        pid_map[curr_id]->page_header[curr_vp]->valid = true;
        pid_map[curr_id]->max_vp += 1;
    
        return addr;
    } else {
        // file backed pages
        // check filename validation
        bool check;
        string true_filename;
        std::tie(check, true_filename) = check_filename(filename);
        if (!check) {
            return nullptr;
        }

        // input filename vp and offset calculate
        unsigned int curr_vp = pid_map[curr_id]->max_vp;

        // Check if (filename, block) already in physical page
        for (auto [key, ppage]:ppn_map) { 
            Header* vp_header = &ppage.header;
            unsigned int ppn = ppage.ppn;
            if (vp_header->is_file_backed) {
                if ( vp_header->filename == true_filename && vp_header->block == (int)block && ppn != 0) {
                    Header* new_header = new Header;
                    pid_map[curr_id]->page_header.push_back(new_header);
                    pid_map[curr_id]->max_vp += 1;
                    pid_map[curr_id]->page_header[curr_vp]->block = block;
                    pid_map[curr_id]->page_header[curr_vp]->filename = true_filename;
                    pid_map[curr_id]->page_header[curr_vp]->is_file_backed = 1;
                    pid_map[curr_id]->page_header[curr_vp]->valid = true;
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
                    
                    ppn_map[ppn].process_id.push_back(curr_id);
                    ppn_map[ppn].vp_index.push_back(curr_vp);
                    return addr;
                }
            }
        }

        Header* new_page = new Header();
        pid_map[curr_id]->page_header.push_back(new_page);
        
        // find the filename, insert into header
        pid_map[curr_id]->page_header[curr_vp]->block = block;
        pid_map[curr_id]->page_header[curr_vp]->filename = true_filename;

        pid_map[curr_id]->page_table.ptes[curr_vp].ppage = 0;
        pid_map[curr_id]->page_table.ptes[curr_vp].read_enable = 0; // for file backed new page should set to read and write to zero since the file content is not already in physical page
        pid_map[curr_id]->page_table.ptes[curr_vp].write_enable = 0;
        pid_map[curr_id]->page_header[curr_vp]->is_file_backed = 1;
        pid_map[curr_id]->max_vp += 1;
        pid_map[curr_id]->page_header[curr_vp]->valid = true;
        return addr;
    }
}


// check if the filename in arena, filename might be a ptr to any part of a filename c string
bool in_arena(const char* filename) {
    unsigned int vp = (uintptr_t)((uintptr_t)filename - (uintptr_t)VM_ARENA_BASEADDR) >> 16;
    if (vp >= vpn_max) return false;
    if (vp >= pid_map[curr_id]->max_vp) return false;
    if (!pid_map[curr_id]->page_header[vp]->valid) return false;
    return true;
}

// Find the empty swap block index that can put in swap page content 
int find_free_swap_block() {
    int i = 0;
    for (bool it:swap_blocks_status) {
        // find the false one, empty one
        if (!it) return i; // false is not occupied
        i++;
    }
    // is fully occupied
    return -1;
}

// for debug purpose
void see_clock_queue() {
    for (auto it = clock_queue.begin(); it != clock_queue.end(); ++it) {
        cout << "ppn: " << (*it)->ppn << "resident: " << (*it)->resident << "dirty: " << (*it)->dirty << "referenced: " << (*it)->referenced << endl;
    }    
}

// see the current physical memory
void see_pysmem() {
    cout << "===================Show physical memory===================" << endl;
    for (int j = 0; j < 4; j++) {
        cout << "phymsm " << j << endl;
        for (int i = 0; i < 64; i++) {
            cout << *((char*)vm_physmem + (j << 16) + i);
        }
        cout << endl;
    }
    cout << "===================End of show physical memory===================" << endl;
}

// set the rbit and wbit and ppage of the virtual page that map to that physical page ppe, if these input params are smaller than 0, then remain the same
void set_vp_from_ppage_entry(PPage_entry* ppe, int r_bit, int w_bit, int ppage) {
    if (!ppe->process_id.empty() && !ppe->vp_index.empty()) {
        int temp_i = 0;
        for (auto it : ppe->process_id) {
            if (r_bit >= 0) pid_map[it]->page_table.ptes[ppe->vp_index[temp_i]].read_enable = (unsigned int)r_bit;
            if (w_bit >= 0) pid_map[it]->page_table.ptes[ppe->vp_index[temp_i]].write_enable = (unsigned int)w_bit;
            if (ppage >= 0) pid_map[it]->page_table.ptes[ppe->vp_index[temp_i]].ppage = (unsigned int)ppage;
            temp_i++;
        }
    }
}

// push the current page into clock queue and update all the bits of virtual page if is filebacked page, update only one vp if is swap backed page, used only for physical page is not resident scenario
void clock_queue_push(PPage_entry* ppe, string filename, int block, int vp) {
    // if is swap back page, then one ppage can only have one vp map to it, clear and reassign is fine
    // if is file back page, then one ppage can have multiple vp map to it, clear and reassign multiple vp pages

    ppe->process_id.clear();
    ppe->vp_index.clear();
    ppe->process_id.push_back(curr_id);// since for file backed page, the ppn is already set in the vm_fault so don't need to worry about duplivcate push (they are outside of condition ppage = 0 on line 782)
    ppe->vp_index.push_back(vp);

    // update all the virtual page in different processes's header the same as the updated one
    if (pid_map[curr_id]->page_header[vp]->is_file_backed) { // read from one vp that map to the physical page
        for (auto [pid, process] : pid_map) {
            int i = 0;
            for (Header* header : process->page_header) {
                if (process->page_table.ptes[i].ppage == 0 &&// TODO: check if set ppage in ptes to zero when ppn is not resident
                    header-> is_file_backed &&
                    header->filename == filename &&
                    block == header-> block) {
                        ppe->process_id.push_back(pid);
                        ppe->vp_index.push_back(i);
                        process->page_table.ptes[i].ppage = ppe->ppn;
                        process->page_table.ptes[i].read_enable = pid_map[curr_id]->page_table.ptes[vp].read_enable;
                        process->page_table.ptes[i].write_enable = pid_map[curr_id]->page_table.ptes[vp].write_enable;
                    }
                i++;
            }
        }
    }

    clock_queue.push_back(ppe);
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
    clock_queue.pop_front();
    temp->resident = false;

    // set all virtual page of this ppn as 0, evict virtual page
    set_vp_from_ppage_entry(temp, -1, -1, 0);
    
    // if is swap backed, if is dirty, write to disk
    if (!temp->header.is_file_backed) {
        if (temp->dirty) {
            // write to swap back block
            // find the swap block that is free
            int new_block;
            if (temp->header.block != -1) {
                new_block = temp->header.block;
            } else {
                new_block = find_free_swap_block();
            }
            file_write(nullptr, new_block, (void*)((uintptr_t)vm_physmem + (temp->ppn << 16)));
            
            swap_blocks_status[new_block] = true;
            pid_map[temp->process_id[0]]->page_header[temp->vp_index[0]]->block = new_block;
            temp->dirty = false;
            set_vp_from_ppage_entry(temp, -1, 0, -1);
        }
    } else {
        if (temp->dirty) {
            // write to file back block
            // find the file block
            file_write(temp->header.filename.c_str(), temp->header.block, (void*)((uintptr_t)vm_physmem + (temp->ppn << 16)));
            temp->dirty = false;
            set_vp_from_ppage_entry(temp, -1, 0, -1);
        }
        // assert(!temp->dirty);
    }
    memset(((char*)vm_physmem + (temp->ppn << 16)), '\0', sizeof(char)*VM_PAGESIZE);
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
        int phy_index = check_phy_mem_empty_slot();
        memset(((char*)vm_physmem + ((phy_index) << 16)), '\0', sizeof(char)*VM_PAGESIZE);
        physical_pages_status[phy_index] = true;
        return (unsigned int)phy_index;
    } else {
        return evict_clock_queue();
    }
}

// check filename in arena and properly stored and return it
std::pair<bool, string> check_filename(const char* filename) {
    // check filename in arena
    if (!in_arena(filename)) return std::make_pair(false, "");

    // check file is readable
    unsigned int vp = (uintptr_t)((uintptr_t)filename - (uintptr_t)VM_ARENA_BASEADDR) >> 16;
    unsigned int offset = (uintptr_t)filename - (vp << 16);
    unsigned int ppn = pid_map[curr_id]->page_table.ptes[vp].ppage;
    string result = "";

    while (vp < vpn_max) {
        // check valid
        if (vp >= pid_map[curr_id]->max_vp) return std::make_pair(false, "");
        if (!pid_map[curr_id]->page_header[vp]->valid) return std::make_pair(false, "");

        // check residence
        if (pid_map[curr_id]->page_table.ptes[vp].read_enable == 0 || ppn == 0) {
            //cout << "go to check vm+fault\n";
            if (vm_fault((const void *)((uintptr_t)(vp << 16) + (uintptr_t)VM_ARENA_BASEADDR), 0) == -1) return std::make_pair(false, "");
        } 

        // check '\0'
        unsigned int ppn_curr = pid_map[curr_id]->page_table.ptes[vp].ppage;
        for (; offset < VM_PAGESIZE; offset++){ 
            unsigned int p_address = (ppn_curr << 16) + offset;
            result +=  ((char *)vm_physmem)[p_address];
            if (((char *)vm_physmem)[p_address] == '\0')  return std::make_pair(true, result);
        }
        offset = 0;
        vp++;
    }
    return std::make_pair(false, "");
}