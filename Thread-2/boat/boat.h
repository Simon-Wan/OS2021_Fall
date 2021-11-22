#ifndef BOAT_H_
#define BOAT_H_

#include<stdio.h>
#include <thread>
#include <mutex>
#include <unistd.h>

#include <condition_variable>

#include "boatGrader.h"

namespace proj2{
class Boat{
public:
	Boat();
    ~Boat(){};
	void begin(int, int, BoatGrader*);

protected:
    int num_adult_O;
    int num_child_O;
    int num_aduly_M;
    int num_child_M;
    std::mutex mtx_naO;
    std::mutex mtx_ncO;
    std::mutex mtx_naM;
    std::mutex mtx_ncM;

    std::condition_variable cv_adult_O;
    std::condition_variable cv_child_O;
    std::condition_variable cv_adult_M;
    std::condition_variable cv_child_M;
    std::mutex mtx_adult_O;
    std::mutex mtx_child_O;
    std::mutex mtx_adult_M;
    std::mutex mtx_child_M;




};
}

#endif // BOAT_H_
