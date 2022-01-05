#ifndef MMA_CLIENT_H
#define MMA_CLIENT_H

#include <memory>
#include <cstdlib>
#include <thread>
#include <chrono>

#include <grpc++/grpc++.h>

#ifdef BAZEL_BUILD
#include "proto/mma.grpc.pb.h"
#else
#include "mma.grpc.pb.h"
#endif
#include "array_list.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using mma::MMA;
using mma::ReadRequest;
using mma::ReadReply;
using mma::WriteRequest;
using mma::WriteReply;
using mma::AllocateRequest;
using mma::AllocateReply;
using mma::FreeRequest;
using mma::FreeReply;


namespace proj4 {

    class MmaClient {
    public:
        MmaClient(std::shared_ptr<Channel> channel): stub_(MMA::NewStub(channel)) {}
        int ReadPage(int array_id, int virtual_page_id, int offset);
        void WritePage(int array_id, int virtual_page_id, int offset, int value);
        ArrayList *Allocate(int sz);
        void Free(ArrayList* arr);

    private:
        std::unique_ptr<MMA::Stub> stub_;
    };
} //namespace proj4

#endif