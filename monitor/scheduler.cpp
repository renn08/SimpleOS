#include <iostream>
#include "thread.h"
#include "disk.h"
#include <queue>
#include <vector>
#include <fstream>
#include <cstring>
#include <cassert>
#include <string>
#include <unordered_map>


using std::cout;
using std::endl;

mutex mutex1;
cv lpnr_cv;
cv queue_full_or_has_request_in_queue_cv;

int num_requester = 0;
int num_requester_alive = 0;
int lpnr = 0;
static int queue_cap;
// current track initialized to 0
int current_track = 0;
bool queue_full = false;
std::vector<bool> current_on_queue;
std::vector<bool> almost_die;
std::unordered_map<char*, int> requests_map;

class Request {
public:
    int track;
    int identity;
};

std::vector<Request> request_queue;

int largest_possible_num_of_request() {
    if (num_requester_alive >= queue_cap) {
        return queue_cap;
    } else {
        return num_requester_alive;
    }
}

void request(void* file) {
    char* file_name = (char*) file;
    // the identity of the requester
    int iden = requests_map[file_name];
    std::string track;
    std::ifstream infile(file_name);

    mutex1.lock();
    
    // begin to request
    while (infile >> track) {
        // cout << "in" << endl;
        // wait if the queue is full, need the server to deal with request
        while ((int)(request_queue.size()) == queue_cap || current_on_queue[iden]) {
            queue_full_or_has_request_in_queue_cv.wait(mutex1);
        }
        if (!current_on_queue[iden]) {
            print_request(iden, std::stoi(track));
            Request request;
            request.identity = iden;
            request.track = std::stoi(track);
            request_queue.push_back(request);
            assert((int)(request_queue.size()) <= queue_cap);
            current_on_queue[iden] = true;
            // done one request issuing, queue might be lpnr signal the lpnr_cv
            if ((int)(request_queue.size()) == largest_possible_num_of_request()) { 
                lpnr_cv.signal();
            }
        }
    }
    almost_die[iden] = true; 
    while (current_on_queue[iden]) {
        queue_full_or_has_request_in_queue_cv.wait(mutex1); 
    }
    // end of the request, ready to exits
    
    
    mutex1.unlock();
    return;
}

void print_queue(std::vector<Request> &a) {
    for (int i = 0; i < (int)(a.size()); ++i) {
        cout << a[i].track << " ";
    }
    cout << endl;
}

void service(void* a) {
    char** argv = (char**) a;
    
    // start the requester threads
    current_on_queue.resize(num_requester);
    num_requester_alive = num_requester;
    // make sure num_requester_alive is true when entering
    for (int i = 0; i < num_requester; ++i) {
        std::ifstream infile_try(argv[i + 2]);
        std::string track_try;
        // if the input file is empty or cannot be opened
        if (!infile_try || !(infile_try >> track_try)) {
            num_requester_alive -= 1;
        } 
    }
    
    for (int i = 0; i < num_requester; ++i) {
        current_on_queue[i] = false;
        almost_die.push_back(false);
        thread((thread_startfunc_t) request, (void *) argv[i + 2]);
    }

    mutex1.lock();
    while(num_requester_alive > 0 && queue_cap != 0) {
        // should only begin handling request when the queue is at the largest possible number of requests
        // calculate the lpnr
        // wait requester to issue request if is not lpnr
        while((int)(request_queue.size()) != largest_possible_num_of_request()) {
            lpnr_cv.wait(mutex1);
        }
        assert((int)(request_queue.size()) == largest_possible_num_of_request());
        assert((int)(request_queue.size()) <= queue_cap);
        int dist = 1000;
        int nearest = 0;
        for (int i = 0; i < (int)(request_queue.size()); ++i) {
            int dist_temp = current_track - request_queue[i].track;
            if (dist_temp < 0) dist_temp = -1 * dist_temp;
            if (dist_temp < dist) {
                dist = dist_temp;
                nearest = i;
            }
        }
     
        int iden = request_queue[nearest].identity;
        int track = request_queue[nearest].track;
        print_service(iden, track);
        request_queue.erase(request_queue.begin() + nearest);
        current_track = track;
        current_on_queue[iden] = false;

        // see if anyone is waiting to be inactive
        if (almost_die[iden]) {
            num_requester_alive -= 1;
        }
        // issue one request and the num of request must be lpnr - 1, call broadcast
        queue_full_or_has_request_in_queue_cv.broadcast();
    }
    mutex1.unlock();
    return;
}

int main(int argc, char** argv) {
    num_requester = argc - 2;
    queue_cap = std::atoi(argv[1]);

    for (int i = 0; i < num_requester; ++i) {
        requests_map.insert({argv[i + 2], i});
    }

    // start the server thread
    cpu::boot((thread_startfunc_t) service, (void *) argv, 0);
}
