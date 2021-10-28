#include <vector>
#include <tuple>

#include <string>   // string
#include <chrono>   // timer
#include <iostream> // cout, endl
#include <algorithm>

#include "lib/utils.h"
#include "lib/model.h" 
#include "lib/embedding.h" 
#include "lib/instruction.h"
#include "q4.h"

namespace proj1 {

void run_one_instruction_test(Instruction inst, EmbeddingHolder* users, EmbeddingHolder* items) {
    switch(inst.order) {
        case INIT_EMB: {
            // We need to init the embedding
            int length = users->get_emb_length();
            Embedding* new_user = new Embedding(length);
            int user_idx = users->append(new_user);
            for (int item_index: inst.payloads) {
                Embedding* item_emb = items->get_embedding(item_index);
                // Call cold start for downstream applications, slow
                EmbeddingGradient* gradient = cold_start(new_user, item_emb);
                users->update_embedding(user_idx, gradient, 0.01);
                delete gradient;
            }
            break;
        }
        case UPDATE_EMB: {
            int user_idx = inst.payloads[0];
            int item_idx = inst.payloads[1];
            int label = inst.payloads[2];
            // You might need to add this state in other questions.
            // Here we just show you this as an example
            // int epoch = -1;
            //if (inst.payloads.size() > 3) {
            //    epoch = inst.payloads[3];
            //}
            Embedding* user = users->get_embedding(user_idx);
            Embedding* item = items->get_embedding(item_idx);
            EmbeddingGradient* gradient = calc_gradient(user, item, label);
            users->update_embedding(user_idx, gradient, 0.01);
            delete gradient;
            gradient = calc_gradient(item, user, label);
            items->update_embedding(item_idx, gradient, 0.001);
            delete gradient;
            break;
        }
        case RECOMMEND: {
            int user_idx = inst.payloads[0];
            Embedding* user = users->get_embedding(user_idx);
            std::vector<Embedding*> item_pool;
            int iter_idx = inst.payloads[1];
            for (unsigned int i = 2; i < inst.payloads.size(); ++i) {
                int item_idx = inst.payloads[i];
                item_pool.push_back(items->get_embedding(item_idx));
            }
            Embedding* recommendation = recommend(user, item_pool);
            recommendation->write_to_stdout();
            break;
        }
    }

}
} // namespace proj1

int main(int argc, char *argv[]) {


    proj1::EmbeddingHolder* users = new proj1::EmbeddingHolder("data/q4.in");
    proj1::EmbeddingHolder* items = new proj1::EmbeddingHolder("data/q4.in");
    proj1::Instructions instructions = proj1::read_instructrions("data/q4_test_instruction.tsv");

    //// begin
    std::vector <std::thread> thread_list;
    std::mutex mtx;
    std::priority_queue<int, std::vector<int>, std::greater<int>> update_queue;
    for (proj1::Instruction inst: instructions) {
        if (inst.order == 1 && inst.payloads.size() > 3) {
            update_queue.push(inst.payloads[3]);
        }
    }
    for (proj1::Instruction inst: instructions) {
        thread_list.push_back(std::thread(proj1::run_one_instruction, inst, users, items, std::ref(mtx), std::ref(update_queue)));
    }
    for (std::thread &each_thread: thread_list) {   //// use '&' to ensure that each_thread can be modified
        each_thread.join();
    }

    int len = instructions.size();
    std::vector<int> perm;
    for (int i = 0; i < len; i++) {
        perm.push_back(i);
    }
    bool check = false;
    do {
        proj1::EmbeddingHolder* users1 = new proj1::EmbeddingHolder("data/q4.in");
        proj1::EmbeddingHolder* items1 = new proj1::EmbeddingHolder("data/q4.in");
        for (int i = 0; i < len; i++){
            proj1::run_one_instruction_test(instructions[perm[i]], users1, items1);
        }
        if (*users1 == *users && *items1 == *items){
            check = true;
            break;
        }
        delete users1;
        delete items1;
    } while(std::next_permutation(perm.begin(), perm.end()));

    if(check)
        printf("Success!\n");
    else
        printf("Failure\n");
    delete users;
    delete items;

    return 0;
}

