#include <thread>
#include <vector>
#include <unistd.h>

#include "boat.h"

namespace proj2{
	
Boat::Boat(){
}

void Boat:: begin(int a, int b, BoatGrader *bg){
    std::vector<std::thread> thread_list;
    for (int i = 0; i < a; i++) {
        thread_list.push_back(std::thread(&Boat::adult, this, bg));
        bg -> initializeAdult();
    }
    for (int i = 0; i < b; i++) {
        thread_list.push_back(std::thread(&Boat::child, this, bg));
        bg -> initializeChild();
    }

    std::unique_lock<std::mutex> lock_child_O;
    cv_child_O.signal()
} 
}