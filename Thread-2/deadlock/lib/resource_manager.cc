#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <condition_variable>
#include "resource_manager.h"

namespace proj2 {

bool ResourceManager::banker(RESOURCE r, int amount, std::thread::id this_id){
    if (this->resource_amount[r] < amount)
        return false;
            mtx_Alloc.lock();
            std::map<std::thread::id, std::map<RESOURCE, int> > Alloc = resource_Alloc;
            mtx_Alloc.unlock();
            mtx_Max.lock();
            std::map<std::thread::id, std::map<RESOURCE, int> > Max = resource_Max;
            mtx_Max.unlock();
            std::map<RESOURCE, int> Avail = resource_amount;
            Avail[r] -= amount;
            Alloc[this_id][r] += amount;
            //printf("This thread: %d, RESOURCE: %d, amount: %d, Avail: %d, Alloc: %d, ", this_id, r, amount, Avail[r], Alloc[this_id][r]);
            std::vector<std::thread::id> UNFINISHED;
            for (auto iter : Max) {
                UNFINISHED.push_back(iter.first);
                //printf("%d,", iter.first);
            }
            //printf("\n");
            while (true) {
            	bool done = true;
            	std::vector<std::thread::id> UNFINISHED_NODES(UNFINISHED);
            	for (auto node : UNFINISHED_NODES) {
            		bool able_to_finish = true;
            		for (RESOURCE t = GPU; t <= NETWORK; t = RESOURCE(t+1)) {
            		    //printf("(%d, %d, %d)", Max[node][t], Alloc[node][t], Avail[t]);
            			if (Max[node][t] - Alloc[node][t] > Avail[t]) {
            			    able_to_finish = false;
            			    //printf("False(%d)!", t);
            			}
					}
					if (able_to_finish) {
						UNFINISHED.erase(std::find(std::begin(UNFINISHED), std::end(UNFINISHED), node));
						for (RESOURCE t = GPU; t <= NETWORK; t = RESOURCE(t+1)) Avail[t] += Alloc[node][t];
						//printf("succeed!");
						done = false;
					}
					//printf("\n");
					//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
				//printf("check done\n");
				if (done) {
				    //printf("done!\n");
				    break;
				}
				//else
				    //printf("not done!\n");
			}
			//printf("check UNFINISHED\n");
			if (UNFINISHED.empty()) {
			    //printf("UNFINISHED clear!\n");
			    return true;
			}
			//else
			    //printf("UNFINISHED not clear!\n");
			return false;
}


int ResourceManager::request(RESOURCE r, int amount) {
    if (amount <= 0)  return 1;

    std::unique_lock<std::mutex> lk(this->resource_mutex);
    auto this_id = std::this_thread::get_id();
    printf("start(%d, %d)\n", this_id, r);
    while (true) {
        if (this->resource_cv.wait_for(
            lk, std::chrono::milliseconds(100),
            [this, r, amount, this_id] {  return this->banker(r, amount, this_id); }
        )) {
            break;
        } else {
            //auto this_id = std::this_thread::get_id();
            /* HINT: If you choose to detect the deadlock and recover,
                     implement your code here to kill and restart threads.
                     Note that you should release this thread's resources
                     properly.
             */
            if (tmgr->is_killed(this_id)) {
                return -1;
            }

        }
    }
    mtx_Alloc.lock();
    //auto this_id = std::this_thread::get_id();
    resource_Alloc[this_id][r] += amount;
    printf("%d gets resource %d with amount %d\n", this_id, r, amount);
    mtx_Alloc.unlock();

    this->resource_amount[r] -= amount;
    this->resource_mutex.unlock();
    printf("return(%d, %d, %d)\n", this_id, r, this->resource_amount[r]);
    return 0;
}

void ResourceManager::release(RESOURCE r, int amount) {
    if (amount <= 0)  return;
    auto this_id = std::this_thread::get_id();
    std::unique_lock<std::mutex> lk(this->resource_mutex);
    this->resource_amount[r] += amount;
    this->resource_Alloc[this_id][r] -= amount;
    printf("%d: release %d with amount %d, remaining %d\n", this_id, r, amount, this->resource_amount[r]);
    this->resource_cv.notify_all();
}

void ResourceManager::budget_claim(std::map<RESOURCE, int> budget) {
    // This function is called when some workload starts.
    // The workload will eventually consume all resources it claims
    auto this_id = std::this_thread::get_id();
    mtx_Max.lock();
    this->resource_Max[this_id] = budget;
    mtx_Max.unlock();
    mtx_Alloc.lock();
    for (RESOURCE r = GPU; r <= NETWORK; r = RESOURCE(r+1)) resource_Alloc[this_id][r] = 0;
    mtx_Alloc.unlock();

}

} // namespace: proj2
