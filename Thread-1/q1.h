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
            ////
            int user_idx = users->append(new_user);
            users->lock_list[user_idx]->lock();      //// acquire new user's lock
            for (int item_index: inst.payloads) {
                bool waiting = true;
                while (waiting) {
                    mtx.lock();
                    waiting = false;
                    if(items->lock_list[item_index]->try_lock()) {  //// acquire item's lock
                    }
                    else
                        waiting = true;
                    mtx.unlock();
                }
                Embedding* item_emb = items->get_embedding(item_index);
                items->lock_list[item_index]->unlock();    //// release this item's lock

                EmbeddingGradient* gradient = cold_start(new_user, item_emb);
                users->update_embedding(user_idx, gradient, 0.01);
            }
            users->lock_list[user_idx]->unlock();    //// release new user's lock

            break;
        }
        case UPDATE_EMB: {
            int user_idx = inst.payloads[0];
            int item_idx = inst.payloads[1];
            int label = inst.payloads[2];
            //// acquire lock for user and item
            bool waiting = true;
            while (waiting) {
                mtx.lock();
                waiting = false;
                if(user_idx >= users->get_n_embeddings()) {     //// check if the user exists
                    waiting = true;
                }
                else {
                    break;
                }
                mtx.unlock();   //// give control to other threads
            }
            mtx.unlock();
            ////

            //// acquire lock for user and item
            waiting = true;
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
            users->lock_list[user_idx]->unlock();    //// release user's lock
            gradient = calc_gradient(item, user, label);
            items->update_embedding(item_idx, gradient, 0.001);
            items->lock_list[item_idx]->unlock();    //// release item's lock
            break;
        }
        default:
            break;
    }

}
} // namespace proj1

