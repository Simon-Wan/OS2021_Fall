#include "memory_manager.h"

#include <fstream>

namespace proj4 {
    PageFrame::PageFrame(){
    }
    int& PageFrame::operator[] (unsigned long idx){
        //each page should provide random access like an array
        return this->mem[idx];
    }
    void PageFrame::WriteDisk(std::string filename) {
        // write page content into disk files
        FILE *fp = fopen(("disk/" + filename).c_str(), "w");
        fwrite(mem, sizeof(mem), 1, fp);
        fclose(fp);
    }
    void PageFrame::ReadDisk(std::string filename) {
        // read page content from disk files
        FILE *fp = fopen(("disk/" + filename).c_str(), "r");
        fread(mem, sizeof(mem), 1, fp);
        fclose(fp);
    }

    PageInfo::PageInfo(){
        this->holder = -1;
        this->virtual_page_id = -1;
    }
    void PageInfo::SetInfo(int cur_holder, int cur_vid){
        //modify the page states
        //you can add extra parameters if needed
        this->holder = cur_holder;
        this->virtual_page_id = cur_vid;
    }
    void PageInfo::ClearInfo(){
        //clear the page states
        //you can add extra parameters if needed
        this->holder = -1;
        this->virtual_page_id = -1;
    }

    int PageInfo::GetHolder(){
        return this->holder;
    }
    int PageInfo::GetVid(){
        return this->virtual_page_id;
    }


    MemoryManager::MemoryManager(size_t sz){
        //mma should build its memory space with given space size
        //you should not allocate larger space than 'sz' (the number of physical pages)
        this->mma_sz = sz;
        this->next_array_id = 0;
        this->mem = new PageFrame[sz];
        this->page_info = new PageInfo[sz];
        this->CLOCK = new bool[sz];
        this->CLOCKidx = 0;
        for (int i = 0; i < sz; i++){
            PageFrame* newPage = new PageFrame;
            this->mem[i] = *newPage;
            PageInfo* newPageInfo = new PageInfo;
            this->page_info[i] = *newPageInfo;
            this->phy_mtx_list.push_back(new std::mutex);
        }
        int free_list_len = sz / 32 + 1;
        this->free_list = new unsigned int[free_list_len];
        for (int i = 0; i < free_list_len; i++){
            this->free_list[i] = 0;
        }
    }
    MemoryManager::~MemoryManager(){
    }
    void MemoryManager::PageOut(int physical_page_id, int holder, int virtual_page_id){
        //swap out the physical page with the indx of 'physical_page_id out' into a disk file
        std::string filename = std::to_string(holder) + "_" + std::to_string(virtual_page_id);
        this->mem[physical_page_id].WriteDisk(filename);
    }
    void MemoryManager::PageIn(int array_id, int virtual_page_id, int physical_page_id){
        //swap the target page from the disk file into a physical page with the index of 'physical_page_id out'
        std::string filename = std::to_string(array_id) + "_" + std::to_string(virtual_page_id);
        this->mem[physical_page_id].ReadDisk(filename);
    }
    void MemoryManager::PageReplace(int array_id, int virtual_page_id){
        //implement your page replacement policy here
        int physical_page_id = -1;
        // Case 1: GOTO free list and find a Pid
        for (int i = 0; i < this->mma_sz; i++){
            if ((this->free_list[i>>5] & (0x8000 >> (i & 0x1f))) == 0){
                physical_page_id = i;
                this->free_list[i>>5] |= (0x8000 >> (i & 0x1f));
                break;
            }
        }
        // Case 2: Use Clock Algorithm to get a Pid
        if (physical_page_id == -1){
        	while (this->CLOCK[this->CLOCKidx] == true) {
        		this->CLOCK[this->CLOCKidx] = false;
        		this->CLOCKidx = (this->CLOCKidx + 1) % this->mma_sz;
			}
			physical_page_id = this->CLOCKidx;
			this->CLOCKidx = (this->CLOCKidx + 1) % this->mma_sz;
        }
        // Got a Pid
        int last_holder = this->page_info[physical_page_id].GetHolder();
        int last_vid = this->page_info[physical_page_id].GetVid();
        int temp_pid = this->page_map[array_id][virtual_page_id];
        this->page_map[array_id][virtual_page_id] = physical_page_id;
        if (last_holder != -1){
            this->vir_mtx_list[last_holder][last_vid]->lock();
            this->page_map[last_holder][last_vid] = -1;
        }
        this->phy_mtx_list[physical_page_id]->lock();
        this->page_info[physical_page_id].SetInfo(array_id, virtual_page_id);
        this->mtx.unlock();
        if (last_holder != -1){ // Need to PageOut
            this->PageOut(physical_page_id, last_holder, last_vid);
            this->vir_mtx_list[last_holder][last_vid]->unlock();
        }
        if (temp_pid == -2){    // -2 indicates that it's a brand new page, no need to PageIn
            for(int i = 0; i < PageSize; i++){
                this->mem[physical_page_id][i] = 0;
            }
        }
        else {
            this->PageIn(array_id, virtual_page_id, physical_page_id);
        }
    }
    int MemoryManager::ReadPage(int array_id, int virtual_page_id, int offset){
        // for arrayList of 'array_id', return the target value on its virtual space
        this->mtx.lock();
        this->vir_mtx_list[array_id][virtual_page_id]->lock();
        int physical_page_id = this->page_map[array_id][virtual_page_id];
        int result = this->mem[physical_page_id][offset];
        if (physical_page_id < 0){  // Need to allocate Pid
            this->PageReplace(array_id, virtual_page_id);
            physical_page_id = this->page_map[array_id][virtual_page_id];
            result = this->mem[physical_page_id][offset];
        }
        else{   // Read on Memory
            this->phy_mtx_list[physical_page_id]->lock();
            this->mtx.unlock();
        }
        this->CLOCK[physical_page_id] = true;
        this->vir_mtx_list[array_id][virtual_page_id]->unlock();
        this->phy_mtx_list[physical_page_id]->unlock();
        return result;
    }
    void MemoryManager::WritePage(int array_id, int virtual_page_id, int offset, int value){
        // for arrayList of 'array_id', write 'value' into the target position on its virtual space
        this->mtx.lock();
        this->vir_mtx_list[array_id][virtual_page_id]->lock();
        int physical_page_id = this->page_map[array_id][virtual_page_id];
        if (physical_page_id < 0){  // Need to allocate Pid
            this->PageReplace(array_id, virtual_page_id);
            physical_page_id = this->page_map[array_id][virtual_page_id];
        }
        else {  // Write on memory
            this->phy_mtx_list[physical_page_id]->lock();
            this->mtx.unlock();
        }
        this->mem[physical_page_id][offset] = value;
        this->CLOCK[physical_page_id] = true;
        this->vir_mtx_list[array_id][virtual_page_id]->unlock();
        this->phy_mtx_list[physical_page_id]->unlock();
    }
    int MemoryManager::Allocate(size_t sz){
        // when an application requires for memory, create an ArrayList and record mappings from its virtual memory space to the physical memory space
        this->mtx.lock();
        // ArrayList* new_array = new ArrayList(sz, this, this->next_array_id);
        std::map<int, int> array_page_map;
        std::vector<std::mutex *> new_array_mtx_list;
        for (int i = 0; i <= (sz - 1) / PageSize; i++) {
            array_page_map[i] = -2;
            new_array_mtx_list.push_back(new std::mutex);
        }
        this->page_map[this->next_array_id] = array_page_map;
        this->next_array_id += 1;
        this->vir_mtx_list.push_back(new_array_mtx_list);
        this->mtx.unlock();
        // return new_array;
        return (this->next_array_id - 1);
    }
    int MemoryManager::Release(int array_id){
        // an application will call release() function when destroying its arrayList
        // release the virtual space of the arrayList and erase the corresponding mappings
        this->mtx.lock();
        int id = array_id;
        int pages = 0;
        // lock all virtual locks
        for(auto lck: this->vir_mtx_list[id]) {
            lck->lock();
        }
        std::map<int, int> array_page_map;
        array_page_map = this->page_map[id];

        for (auto it: array_page_map) {
            int physical_page_id = it.second;
            pages ++;
            if (physical_page_id != -1){

                this->phy_mtx_list[physical_page_id]->lock();
            	this->CLOCK[physical_page_id] = false;
            	for (int j = 0; j < PageSize; j++) {
                	this->mem[physical_page_id][j] = 0;
				}
                this->page_info[physical_page_id].ClearInfo();
                this->phy_mtx_list[physical_page_id]->unlock();
            }
        }
        this->page_map.erase(id);
        // unlock all virtual locks
        for(auto lck: this->vir_mtx_list[id]) {
            lck->unlock();
        }
        this->mtx.unlock();

        return pages;
    }
} // namespce: proj3
