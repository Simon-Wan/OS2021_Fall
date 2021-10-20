#include <vector>
#include <tuple>

#include <string>   // string
#include <chrono>   // timer
#include <iostream> // cout, endl

#include <mutex>    // lock
#include <thread>   // thread

#include "lib/utils.h"
#include "lib/model.h"
#include "lib/embedding.h"
#include "lib/instruction.h"

namespace proj1 {

void run_one_instruction(Instruction inst, EmbeddingHolder* users, EmbeddingHolder* items, std::mutex& mtx) {
    switch(inst.order) {
        case INIT_EMB: {
            // We need to init the embedding
            int length = users->get_emb_length();
            Embedding* new_user = new Embedding(length);
            //// todo: acquire items' lock (finish)
            bool waiting = true;
            while (waiting) {
                mtx.lock();
                waiting = false;
                for (int item_index: inst.payloads) {
                    if(items->lock_list[item_index]->try_lock()) {
                        items->lock_list[item_index]->unlock();
                    }
                    else {
                        waiting = true;
                        break;
                    }
                }
                if (!waiting) {
                    for (int item_index: inst.payloads) {
                        items->lock_list[item_index]->lock();
                    }
                    break;
                }
                mtx.unlock();   //// give control to other threads
            }
            ////
            int user_idx = users->append(new_user);     //// todo: add lock for new user (finish)
            //// begin
            users->lock_list[user_idx]->lock();      //// acquire new user's lock
            mtx.unlock();       //// unlock mutex after all user & item locks are locked
            //// end
            for (int item_index: inst.payloads) {
                Embedding* item_emb = items->get_embedding(item_index);
                //// begin
                items->lock_list[item_index]->unlock();    //// release this item's lock
                //// end
                EmbeddingGradient* gradient = cold_start(new_user, item_emb);
                users->update_embedding(user_idx, gradient, 0.01);
            }
            //// begin
            users->lock_list[user_idx]->unlock();    //// release new user's lock
            //// end
            break;
        }
        case UPDATE_EMB: {
            int user_idx = inst.payloads[0];
            int item_idx = inst.payloads[1];
            int label = inst.payloads[2];
            //// todo: acquire lock for user and item (finish)
            bool waiting = true;
            while (waiting) {
                mtx.lock();
                waiting = false;
                if(items->lock_list[item_idx]->try_lock()) {
                    items->lock_list[item_idx]->unlock();
                }
                else {
                    waiting = true;
                }
                if(users->lock_list[user_idx]->try_lock()) {
                    users->lock_list[user_idx]->unlock();
                }
                else {
                    waiting = true;
                }
                if (!waiting) {
                    items->lock_list[item_idx]->lock();
                    users->lock_list[user_idx]->lock();
                    break;
                }
                mtx.unlock();   //// give control to other threads
            }
            mtx.unlock();       //// unlock mutex after all user & item locks are locked
            ////

            Embedding* user = users->get_embedding(user_idx);
            Embedding* item = items->get_embedding(item_idx);
            EmbeddingGradient* gradient = calc_gradient(user, item, label);
            users->update_embedding(user_idx, gradient, 0.01);
            //// begin
            users->lock_list[user_idx]->unlock();    //// release user's lock
            //// end
            gradient = calc_gradient(item, user, label);
            items->update_embedding(item_idx, gradient, 0.001);
            //// begin
            items->lock_list[item_idx]->unlock();    //// release item's lock
            //// end
            break;
        }
        default:
            break;
    }

}
} // namespace proj1

int main(int argc, char *argv[]) {

    proj1::EmbeddingHolder* users = new proj1::EmbeddingHolder("data/q1.in");
    proj1::EmbeddingHolder* items = new proj1::EmbeddingHolder("data/q1.in");
    proj1::Instructions instructions = proj1::read_instructrions("data/q1_instruction.tsv");
    {
    proj1::AutoTimer timer("q1");  // using this to print out timing of the block
    // Run all the instructions

    //// begin
    std::vector <std::thread> thread_list;
    std::mutex mtx;
    for (proj1::Instruction inst: instructions) {
        thread_list.push_back(std::thread(proj1::run_one_instruction, inst, users, items, std::ref(mtx)));
    }
    for (std::thread &each_thread: thread_list) {   //// use '&' to ensure that each_thread can be modified
        each_thread.join();
    }
    //// end

    }

    // Write the result
    users->write_to_stdout();
    items->write_to_stdout();

    return 0;
}
