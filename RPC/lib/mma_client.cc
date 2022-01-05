#include "mma_client.h"
#include <chrono>
#include <cstdlib>
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>

#ifdef BAZEL_BUILD
#include "proto/mma.grpc.pb.h"
#else
#include "mma.grpc.pb.h"
#endif

namespace proj4{

int MmaClient::ReadPage(int array_id, int virtual_page_id, int offset) {
    // Data we are sending to the server.
    ReadRequest request;
    request.set_array_id(array_id);
    request.set_virtual_page_id(virtual_page_id);
    request.set_offset(offset);


    // Container for the data we expect from the server.
    ReadReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->ReadPage(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.result();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
}

void MmaClient::WritePage(int array_id, int virtual_page_id, int offset, int value) {
    // Data we are sending to the server.
    WriteRequest request;
    request.set_array_id(array_id);
    request.set_virtual_page_id(virtual_page_id);
    request.set_offset(offset);
    request.set_value(value);


    // Container for the data we expect from the server.
    WriteReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->WritePage(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return;
    }
}

ArrayList *MmaClient::Allocate(int sz) {
    do {
    // Data we are sending to the server.
    AllocateRequest request;
    request.set_sz(sz);


    // Container for the data we expect from the server.
    AllocateReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Allocate(&context, request, &reply);

    int array_id = reply.array_id();
    // Act upon its status.
    if (status.ok()) {
      return new ArrayList(sz, this, array_id);
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    } while (true);
}

void MmaClient::Free(ArrayList *arr) {
    // Data we are sending to the server.
    FreeRequest request;
    int array_id = arr->array_id;
    request.set_array_id(array_id);


    // Container for the data we expect from the server.
    FreeReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Free(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return;
    }
}

}
