#include <vector>
#include <tuple>
#include <map>
#include <utility>

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

void cold_start_with_result(std::pair<EmbeddingGradient*, bool> &gradient, Embedding* newUser, Embedding* item) {
	gradient.first = cold_start(newUser, item);
	gradient.second = true;     //// indicate whether the calculation is finished
}


void run_one_instruction(Instruction inst, EmbeddingHolder* users, EmbeddingHolder* items, std::mutex& mtx) {
    switch(inst.order) {
        case INIT_EMB: {
            // We need to init the embedding
            int length = users->get_emb_length();
            Embedding* new_user = new Embedding(length);
            //// acquire items' lock
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
            int user_idx = users->append(new_user);     //// add lock for new user

            users->lock_list[user_idx]->lock();         //// acquire new user's lock
            mtx.unlock();       //// unlock mutex after all user & item locks are locked

            std::vector <std::thread> inner_thread_list;    //// create inner thread list
            std::map <int, std::pair<EmbeddingGradient*, bool> > embedding_gradient_map;	    //// map from item_idx to its gradient
            for (int item_index: inst.payloads) {
            	embedding_gradient_map[item_index] = std::pair<EmbeddingGradient*, bool> (nullptr, false);
			}
            for (int item_index: inst.payloads) {		    //// create a thread for each item_idx
                Embedding* item_emb = items->get_embedding(item_index);
                items->lock_list[item_index]->unlock();     //// release this item's lock
                inner_thread_list.push_back(std::thread(cold_start_with_result, std::ref(embedding_gradient_map[item_index]), new_user, item_emb));
            }
            for (std::thread &each_thread: inner_thread_list) {
            	each_thread.join();
			}
			while(true) {       //// update the embedding of new user
				bool ready = true;
				for (int item_index: inst.payloads) {
					if (embedding_gradient_map[item_index].second == false)
					    ready = false;
				}
				if (ready) {
					for (int item_index: inst.payloads) {
						users->update_embedding(user_idx, embedding_gradient_map[item_index].first, 0.01);
					}
					break;
				}
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

int main(int argc, char *argv[]) {

    proj1::EmbeddingHolder* users = new proj1::EmbeddingHolder("data/q2.in");
    proj1::EmbeddingHolder* items = new proj1::EmbeddingHolder("data/q2.in");
    proj1::Instructions instructions = proj1::read_instructrions("data/q2_instruction.tsv");
    {
    proj1::AutoTimer timer("q2");  // using this to print out timing of the block
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
