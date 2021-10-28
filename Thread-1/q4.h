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

void process_item_and_get_gradient(std::pair<EmbeddingGradient*, bool> &gradient, EmbeddingHolder* items, Embedding* newUser, int item_index, std::mutex& mtx){
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

    gradient.first = cold_start(newUser, item_emb);
	gradient.second = true;     //// indicate whether the calculation is finished
}

bool update_valid(int epoch, std::priority_queue<int, std::vector<int>, std::greater<int>>& update_queue) {
    if (update_queue.top() == epoch) {
        update_queue.pop();
        return true;
    }
    else
        return false;
}

void run_one_instruction(Instruction inst, EmbeddingHolder* users, EmbeddingHolder* items, std::mutex& mtx, std::priority_queue<int, std::vector<int>, std::greater<int>>& update_queue) {
    switch(inst.order) {
        case INIT_EMB: {
            int length = users->get_emb_length();
            Embedding* new_user = new Embedding(length);
            ////

            std::vector <std::thread> inner_thread_list;    //// create inner thread list
            std::map <int, std::pair<EmbeddingGradient*, bool> > embedding_gradient_map;	    //// map from item_idx to its gradient
            for (int item_index: inst.payloads) {
            	embedding_gradient_map[item_index] = std::pair<EmbeddingGradient*, bool> (nullptr, false);
			}


            int user_idx = users->append(new_user);
            users->lock_list[user_idx]->lock();      //// acquire new user's lock

            for (int item_index: inst.payloads) {		    //// create a thread for each item_idx
                inner_thread_list.push_back(std::thread(process_item_and_get_gradient, std::ref(embedding_gradient_map[item_index]), items, new_user, item_index, std::ref(mtx)));
            }
            for (std::thread &each_thread: inner_thread_list) {
            	each_thread.join();
			}

			for (int item_index: inst.payloads) {			//// update embedding of the user
				users->update_embedding(user_idx, embedding_gradient_map[item_index].first, 0.01);
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
                if (!waiting && update_valid(epoch, update_queue)) {
                    items->lock_list[item_idx]->lock();
                    users->lock_list[user_idx]->lock();
                    //printf("unlock:(%d,%d)\n",user_idx,item_idx);
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
        case RECOMMEND: {
            int user_idx = inst.payloads[0];
            std::vector<int> item_idxes;
        	std::vector<Embedding*> item_pool;
        	int iter_idx = inst.payloads[1];
        	for (unsigned int i = 2; i < inst.payloads.size(); ++i) {
        		item_idxes.push_back(inst.payloads[i]);
			}
			bool waiting = true;
			while (waiting) {
				mtx.lock();
				waiting = false;
				if (users->lock_list[user_idx]->try_lock()) {
					users->lock_list[user_idx]->unlock();
				}
				else {
					waiting = true;
				}
				for (int item_idx : item_idxes) {
					if (items->lock_list[item_idx]->try_lock()) {
						items->lock_list[item_idx]->unlock();
					}
					else {
						waiting = true;
						break;
					}
				}
				if (!update_queue.empty() && iter_idx >= update_queue.top()) {		//// correct position for recommend instructions
					waiting = true;
				}
				if (!waiting) {
					users->lock_list[user_idx]->lock();
					for (int item_idx : item_idxes) {
						items->lock_list[item_idx]->try_lock();
					}
				}
				mtx.unlock();
			}
			mtx.unlock();

        	for (int item_idx : item_idxes) {
            	item_pool.push_back(items->get_embedding(item_idx));
            	items->lock_list[item_idx]->unlock();
           	}
           	Embedding* user = users->get_embedding(user_idx);
           	users->lock_list[user_idx]->unlock();

        	Embedding* recommendation = recommend(user, item_pool);
        	mtx.lock();
        	recommendation->write_to_stdout();		//promise that stdout() is atomic
           	mtx.unlock();
            break;
        }
    }

}
} // namespace proj1
