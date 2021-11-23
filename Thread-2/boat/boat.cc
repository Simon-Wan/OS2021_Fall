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
    for (std::thread &each_thread: thread_list) {
        each_thread.detach();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    cv_child_O.notify_one();
    std::unique_lock<std::mutex> lock_finish(mtx_finish);
    cv_finish.wait(lock_finish);
}

void Boat:: adult(BoatGrader *bg){
    mtx_naO.lock();
    num_adult_O += 1;
    mtx_naO.unlock();

    std::unique_lock<std::mutex> lock_adult_O(mtx_adult_O);
    // printf("An adult starts waiting\n");
    cv_adult_O.wait(lock_adult_O);
    if (lock_adult_O.owns_lock()) {
        lock_adult_O.unlock();
    }
    // printf("An adult is woken up\n");
    mtx_pilot.lock();
    pilot = true;
    mtx_pilot.unlock();

    mtx_naO.lock();
    num_adult_O -= 1;
    mtx_naO.unlock();

    bg->AdultRowToMolokai();

    mtx_pilot.lock();
    mtx_rider.lock();
    pilot = false;
    rider = false;
    mtx_rider.unlock();
    mtx_pilot.unlock();

    cv_child_M.notify_one();

}

void Boat:: child(BoatGrader *bg){
    mtx_ncO.lock();
    num_child_O += 1;
    mtx_ncO.unlock();

    std::unique_lock<std::mutex> lock_child_O(mtx_child_O);
    // printf("A child starts waiting\n");
    cv_child_O.wait(lock_child_O);
    bool at_O = true;
    while (true){
        if (lock_child_O.owns_lock()) {
            lock_child_O.unlock();
        }
        // printf("A child is woken up\n");
        if (at_O) {
            bool check_pilot;
            mtx_pilot.lock();
            check_pilot = pilot;
            mtx_pilot.unlock();
            if (!check_pilot){
                // become a pilot
                mtx_pilot.lock();
                pilot = true;
                mtx_pilot.unlock();
                int remaining_child;
                int remaining_adult;
                mtx_ncO.lock();
                num_child_O -= 1;
                remaining_child = num_child_O;
                mtx_ncO.unlock();
                mtx_naO.lock();
                remaining_adult = num_adult_O;
                mtx_naO.unlock();

                if (remaining_child > 0) {
                    // call for a passenger
                    mtx_sailing.lock();
                    cv_child_O.notify_one();
                    std::unique_lock<std::mutex> lock_aboard(mtx_rider);
                    while (!rider) {
                        cv_aboard.wait(lock_aboard);
                    }
                    lock_aboard.unlock();
                    bg->ChildRowToMolokai();
                    mtx_sailing.unlock();

                    std::unique_lock<std::mutex> lock_ashore(mtx_rider);
                    while (rider) {
                        cv_ashore.wait(lock_ashore);
                    }
                    lock_ashore.unlock();
                    // row back to Oahu

                    bg->ChildRowToOahu();

                    mtx_ncO.lock();
                    num_child_O += 1;
                    mtx_ncO.unlock();
                    mtx_pilot.lock();
                    pilot = false;
                    mtx_pilot.unlock();

                }
                else if (remaining_adult > 0){
                    // give up pilot and transfer to an adult
                    mtx_ncO.lock();
                    num_child_O += 1;
                    mtx_ncO.unlock();

                    mtx_pilot.lock();
                    pilot = false;
                    mtx_pilot.unlock();

                    cv_adult_O.notify_one();
                    lock_child_O.lock();
                    cv_child_O.wait(lock_child_O);

                }

                else {
                    // wait to check if finish
                    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                    mtx_ncO.lock();
                    remaining_child = num_child_O;
                    mtx_ncO.unlock();
                    mtx_naO.lock();
                    remaining_adult = num_adult_O;
                    mtx_naO.unlock();
                    if (remaining_child == 0 && remaining_adult == 0) {
                        bg->ChildRowToMolokai();

                        mtx_pilot.lock();
                        pilot = false;
                        mtx_pilot.unlock();
                        // finish
                        cv_finish.notify_one();

                    }
                    else {
                        mtx_ncO.lock();
                        num_child_O += 1;
                        mtx_ncO.unlock();

                        mtx_pilot.lock();
                        pilot = false;
                        mtx_pilot.unlock();
                    }

                }
            }
            else {
                // become a rider
                mtx_ncO.lock();
                num_child_O -= 1;
                mtx_ncO.unlock();
                mtx_rider.lock();
                rider = true;
                mtx_rider.unlock();
                cv_aboard.notify_one();
                mtx_sailing.lock();
                bg->ChildRideToMolokai();
                mtx_sailing.unlock();

                mtx_rider.lock();
                rider = false;
                mtx_rider.unlock();
                at_O = false;
                cv_ashore.notify_one();

                std::unique_lock<std::mutex> lock_child_M(mtx_child_M);
                cv_child_M.wait(lock_child_M);
                if (lock_child_M.owns_lock()) {
                    lock_child_M.unlock();
                }
            }
        }
        else {
            // row back to Oahu

            mtx_pilot.lock();
            pilot = true;
            mtx_pilot.unlock();

            bg->ChildRowToOahu();

            at_O = true;

            mtx_ncO.lock();
            num_child_O += 1;
            mtx_ncO.unlock();

            mtx_pilot.lock();
            pilot = false;
            mtx_pilot.unlock();
        }
    }


}
}