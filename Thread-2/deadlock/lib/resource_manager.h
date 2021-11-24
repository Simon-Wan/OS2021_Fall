#ifndef DEADLOCK_LIB_RESOURCE_MANAGER_H_
#define DEADLOCK_LIB_RESOURCE_MANAGER_H_

#include <map>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "thread_manager.h"

namespace proj2 {

enum RESOURCE {
    GPU = 0,
    MEMORY,
    DISK,
    NETWORK
};

class ResourceManager {
public:
    ResourceManager(ThreadManager *t, std::map<RESOURCE, int> init_count): \
        resource_amount(init_count), tmgr(t) {}
    void budget_claim(std::map<RESOURCE, int> budget);
    int request(RESOURCE, int amount);
    void release(RESOURCE, int amount);
    bool banker(RESOURCE, int, std::thread::id);
private:
	std::map<std::thread::id, std::map<RESOURCE, int> > resource_Alloc;
	std::map<std::thread::id, std::map<RESOURCE, int> > resource_Max;
	std::mutex mtx_Alloc;
	std::mutex mtx_Max;
	std::mutex mtx_Banker;

    std::map<RESOURCE, int> resource_amount;
    std::mutex resource_mutex;
    std::condition_variable resource_cv;
    ThreadManager *tmgr;
};

}  // namespce: proj2

#endif
