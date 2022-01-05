#include "mma_server.h"


namespace proj4{

std::unique_ptr<Server> server;

Status MmaServiceImpl::ReadPage(ServerContext* context, const ReadRequest* request, ReadReply* reply) {
    int array_id = request->array_id();
    int virtual_page_id = request->virtual_page_id();
    int offset = request->offset();
    int result = mma->ReadPage(array_id, virtual_page_id, offset);
    reply->set_result(result);
    return Status::OK;
}

Status MmaServiceImpl::WritePage(ServerContext* context, const WriteRequest* request, WriteReply* reply) {
    int array_id = request->array_id();
    int virtual_page_id = request->virtual_page_id();
    int offset = request->offset();
    int value = request->value();
    mma->WritePage(array_id, virtual_page_id, offset, value);
    reply->set_message(1);
    return Status::OK;
}

Status MmaServiceImpl::Allocate(ServerContext* context, const AllocateRequest* request, AllocateReply* reply) {
    if (count == max_vir_pages)
        return Status::CANCELLED;
    int sz = request->sz();
    int array_id = mma->Allocate(sz);
    reply->set_array_id(array_id);
    if (count != -1) {
        std::unique_lock<std::mutex> lock(mtx);
        count += (sz - 1) / PageSize + 1;
    }
    return Status::OK;
}

Status MmaServiceImpl::Free(ServerContext* context, const FreeRequest* request, FreeReply* reply) {
    int array_id = request->array_id();
    int release_pages = mma->Release(array_id);
    reply->set_message(1);
    count -= release_pages;
    return Status::OK;
}

void MmaServiceImpl::newMma(int mma_sz) {
    mma = new proj4::MemoryManager(mma_sz);
    count = -1;
}

void MmaServiceImpl::newMma(int mma_sz, int max) {
    mma = new proj4::MemoryManager(mma_sz);
    max_vir_pages = max;
    count = 0;
}

void RunServerUL(size_t phy_page_num){
    std::string server_address("0.0.0.0:50051");
    MmaServiceImpl service;
    service.newMma(phy_page_num);

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    // std::unique_ptr<Server> server(builder.BuildAndStart());
    server = std::unique_ptr<Server>(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

void RunServerL(size_t phy_page_num, size_t max_vir_page_num){
    std::string server_address("0.0.0.0:50051");
    MmaServiceImpl service;
    service.newMma(phy_page_num, max_vir_page_num);

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    // std::unique_ptr<Server> server(builder.BuildAndStart());
    server = std::unique_ptr<Server>(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

void ShutdownServer(){
    server->Shutdown();
}

}
