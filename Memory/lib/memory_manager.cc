#include "memory_manager.h"

#include "array_list.h"

#include <fstream>

namespace proj3 {
    PageFrame::PageFrame(){
    }
    int& PageFrame::operator[] (unsigned long idx){
        //each page should provide random access like an array
        return this->mem[idx];
    }
    void PageFrame::WriteDisk(std::string filename) {
        // write page content into disk files
        // TODO done
        std::fstream outFile;
        outFile.open("./disk/" + filename, std::ios::out|std::ios::trunc);
        if (outFile.fail()) {
            printf("outFile opening failed.\n");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < PageSize; i++){
            outFile << this->mem[i] << std::endl;
        }
        outFile.close();
    }
    void PageFrame::ReadDisk(std::string filename) {
        // read page content from disk files
        // TODO done
        std::fstream inFile;
        inFile.open("./disk/" + filename, std::ios::in);
        if (inFile.fail()) {
            printf("inFile opening failed.\n");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < PageSize; i++){
            inFile >> this->mem[i];
        }
        inFile.close();
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
        for (int i = 0; i < sz; i++){
            PageFrame* newPage = new PageFrame;
            this->mem[i] = *newPage;
            PageInfo* newPageInfo = new PageInfo;
            this->page_info[i] = *newPageInfo;
        }
        int free_list_len = sz / 32 + 1;
        this->free_list = new unsigned int[free_list_len];
    }
    MemoryManager::~MemoryManager(){
    }
    void MemoryManager::PageOut(int physical_page_id){
        //swap out the physical page with the indx of 'physical_page_id out' into a disk file
        int holder = this->page_info[physical_page_id].GetHolder();
        int virtual_page_id = this->page_info[physical_page_id].GetVid();
        std::string filename = std::to_string(holder) + "_" + std::to_string(virtual_page_id) + ".txt";
        this->mem[physical_page_id].WriteDisk(filename);
        this->page_map[holder][virtual_page_id] = -1;

    }
    void MemoryManager::PageIn(int array_id, int virtual_page_id, int physical_page_id){
        //swap the target page from the disk file into a physical page with the index of 'physical_page_id out'
        this->page_info[physical_page_id].SetInfo(array_id, virtual_page_id);
        if (this->page_map[array_id][virtual_page_id] == -2) {
            for(int i = 0; i < PageSize; i++){
                this->mem[physical_page_id][i] = 0;
            }
            return;
        }
        std::string filename = std::to_string(array_id) + "_" + std::to_string(virtual_page_id) + ".txt";
        this->mem[physical_page_id].ReadDisk(filename);
    }
    void MemoryManager::PageReplace(int array_id, int virtual_page_id){
        //implement your page replacement policy here
        int physical_page_id = -1;
        for (int i = 0; i < this->mma_sz; i++){
            if ((this->free_list[i>>5] & (0x8000 >> (i & 0x1f))) == 0){
                physical_page_id = i;
                this->free_list[i>>5] |= (0x8000 >> (i & 0x1f));
                break;
            }
        }   // TODO: efficiency
        if (physical_page_id == -1){
            std::pair<int, int> chosen_page = this->FIFO_queue.front();
            this->FIFO_queue.pop();
            physical_page_id = this->page_map[chosen_page.first][chosen_page.second];
        }
        this->FIFO_queue.push(std::make_pair(array_id, virtual_page_id));
        // TODO: Replace
        int last_holder = this->page_info[physical_page_id].GetHolder();
        //printf("%d_%d\n",last_holder, this->page_info[physical_page_id].GetVid());
        if (last_holder != -1){
            this->PageOut(physical_page_id);
        }
        this->PageIn(array_id, virtual_page_id, physical_page_id);
        this->page_map[array_id][virtual_page_id] = physical_page_id;
    }
    int MemoryManager::ReadPage(int array_id, int virtual_page_id, int offset){
        // for arrayList of 'array_id', return the target value on its virtual space
        int physical_page_id = this->page_map[array_id][virtual_page_id];
        if (physical_page_id < 0){
            this->PageReplace(array_id, virtual_page_id);
            physical_page_id = this->page_map[array_id][virtual_page_id];
        }
        return this->mem[physical_page_id][offset];
    }
    void MemoryManager::WritePage(int array_id, int virtual_page_id, int offset, int value){
        // for arrayList of 'array_id', write 'value' into the target position on its virtual space
        int physical_page_id = this->page_map[array_id][virtual_page_id];
        if (physical_page_id < 0){
            this->PageReplace(array_id, virtual_page_id);
            physical_page_id = this->page_map[array_id][virtual_page_id];
        }
        this->mem[physical_page_id][offset] = value;
    }
    ArrayList* MemoryManager::Allocate(size_t sz){
        // when an application requires for memory, create an ArrayList and record mappings from its virtual memory space to the physical memory space
        ArrayList* new_array = new ArrayList(sz, this, this->next_array_id);
        std::map<int, int> array_page_map;
        for (int i = 0; i <= (sz - 1) / PageSize; i++) {
            array_page_map[i] = -2;
        }
        this->page_map[this->next_array_id] = array_page_map;
        this->next_array_id += 1;
        return new_array;
    }
    void MemoryManager::Release(ArrayList* arr){
        // an application will call release() function when destroying its arrayList
        // release the virtual space of the arrayList and erase the corresponding mappings
        // TODO
        int id = arr->array_id;
        int sz = arr->size;
        std::map<int, int> array_page_map;
        array_page_map = this->page_map[id];
        for (int i = 0; i <= (sz - 1) / PageSize; i++) {
            int physical_page_id = array_page_map[i];
            if (physical_page_id != -1){
                this->page_info[physical_page_id].ClearInfo();
            }
        }
        this->page_map.erase(id);

        int queue_size = this->FIFO_queue.size();
        for (int i = 0; i < queue_size; i++){
            std::pair<int, int> chosen_page = this->FIFO_queue.front();
            this->FIFO_queue.pop();
            if (chosen_page.first != id) {
                this->FIFO_queue.push(chosen_page);
            }
        }
    }
} // namespce: proj3