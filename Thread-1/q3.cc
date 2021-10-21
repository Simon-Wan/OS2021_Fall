#include <vector>
#include <tuple>
#include <map>
#include <utility>

#include <string>   // string
#include <chrono>   // timer
#include <iostream> // cout, endl

#include <mutex>    // lock
#include <thread>   // thread
#include <queue>

#include "lib/utils.h"
#include "lib/model.h"
#include "lib/embedding.h"
#include "lib/instruction.h"

namespace proj1 {

void cold_start_with_result(std::pair<EmbeddingGradient*, bool> &gradient, Embedding* newUser, Embedding* item) {
	gradient.first = cold_start(newUser, item);
	gradient.second = true;     //// indicate whether the calculation is finished
}

bool update_valid(int epoch, std::priority_queue<int, std::vector<int>, std::greater<int>>& update_queue) {
    if (update_queue.top() == epoch) {
        printf("%d\n",epoch);
        update_queue.pop();
        return true;
    }
    else
        return false;
}

void run_one_instruction(Instruction inst, EmbeddingHolder* users, EmbeddingHolder* items, std::mutex& mtx, std::priority_queue<int, std::vector<int>, std::greater<int>>& update_queue) {
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

            int epoch = -1;
            if (inst.payloads.size() > 3) {
                epoch = inst.payloads[3];
            }
            //// push (user, item) in instruction into update queue
            bool waiting = true;
            while (waiting) {

                mtx.lock();
                waiting = false;
                if(user_idx >= users->get_n_embeddings()) {     //// check if the user exists
                    waiting = true;     //todo
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
                if (!waiting && update_valid(epoch, update_queue)) {
                    items->lock_list[item_idx]->lock();
                    users->lock_list[user_idx]->lock();
                    printf("unlock:(%d,%d)\n",user_idx,item_idx);
                    break;
                }
                else {
                    waiting = true;
                }
                mtx.unlock();   //// give control to other threads
            }
            mtx.unlock();       //// unlock mutex after all user & item locks are locked
            ////

            Embedding* user = users->get_embedding(user_idx);
            Embedding* item = items->get_embedding(item_idx);
            EmbeddingGradient* gradient = calc_gradient(user, item, label);
            users->update_embedding(user_idx, gradient, 0.01);
            users->lock_list[user_idx]->unlock();   //// release user's lock

            gradient = calc_gradient(item, user, label);
            items->update_embedding(item_idx, gradient, 0.001);
            items->lock_list[item_idx]->unlock();   //// release item's lock

            break;
        }
        default:
            break;
    }

}
} // namespace proj1

int main(int argc, char *argv[]) {
    proj1::EmbeddingHolder* users = new proj1::EmbeddingHolder("data/q3.in");
    proj1::EmbeddingHolder* items = new proj1::EmbeddingHolder("data/q3.in");

    proj1::Instructions instructions = proj1::read_instructrions("data/q3_instruction.tsv");

    {
    proj1::AutoTimer timer("q3");  // using this to print out timing of the block
    // Run all the instructions

    //// begin
    std::vector <std::thread> thread_list;
    std::mutex mtx;
    std::priority_queue<int, std::vector<int>, std::greater<int>> update_queue;
    for (proj1::Instruction inst: instructions) {
        if (inst.order == 1 && inst.payloads.size() > 3) {
            update_queue.push(inst.payloads[3]);
            printf("%d",inst.payloads[3]);
        }
    }
    printf("\n");
    for (proj1::Instruction inst: instructions) {

        thread_list.push_back(std::thread(proj1::run_one_instruction, inst, users, items, std::ref(mtx), std::ref(update_queue)));
    }

    for (std::thread &each_thread: thread_list) {   //// use '&' to ensure that each_thread can be modified
        each_thread.join();
    }
    //// end

    }

    // Write the result
    //users->write_to_stdout();
    //items->write_to_stdout();

    return 0;
}
