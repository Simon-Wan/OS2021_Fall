#include "array_list.h"

#include "memory_manager.h"

namespace proj3 {
    ArrayList::ArrayList(size_t sz, MemoryManager* cur_mma, int id){
        this->size = sz;
        this->mma = cur_mma;
        this->array_id = id;
    }
    int ArrayList::Read (unsigned long idx){
        //read the value in the virtual index of 'idx' from mma's memory space
        unsigned vir_page_num = idx / PageSize;
        unsigned vir_page_offset = idx % PageSize;
        return this->mma->ReadPage(this->array_id, vir_page_num, vir_page_offset);
    }
    void ArrayList::Write (unsigned long idx, int value){
        //write 'value' in the virtual index of 'idx' into mma's memory space
        unsigned vir_page_num = idx / PageSize;
        unsigned vir_page_offset = idx % PageSize;
        this->mma->WritePage(this->array_id, vir_page_num, vir_page_offset, value);
    }
    ArrayList::~ArrayList(){
    }
} // namespce: proj3