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
	void adult(BoatGrader*);
	void child(BoatGrader*);

protected:
    int num_adult_O = 0;
    int num_child_O = 0;
    std::mutex mtx_naO;
    std::mutex mtx_ncO;

    int num_adult_M = 0;
    int num_child_M = 0;
    std::mutex mtx_naM;
    std::mutex mtx_ncM;

    std::condition_variable cv_adult_O;
    std::condition_variable cv_child_O;
    std::condition_variable cv_child_M;
    std::mutex mtx_adult_O;
    std::mutex mtx_child_O;
    std::mutex mtx_child_M;

    bool pilot = false;
    std::mutex mtx_pilot;
    bool rider = false;
    std::mutex mtx_rider;

    std::condition_variable cv_aboard;
    std::condition_variable cv_ashore;

    std::mutex mtx_sailing;

    std::condition_variable cv_ready;
    std::mutex mtx_ready;
    std::condition_variable cv_finish;
    std::mutex mtx_finish;

    int total_adult;
    int total_child;

    int ready_adult;
    int ready_child;

};
}

#endif // BOAT_H_
