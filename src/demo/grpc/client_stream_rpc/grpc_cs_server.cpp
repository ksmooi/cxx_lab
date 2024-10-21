#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "sum.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReader;
using sum::SumService;
using sum::NumberRequest;
using sum::SumReply;

// Logic and data behind the server's behavior.
class SumServiceImpl final : public SumService::Service {
public:
    // Override the ComputeSum RPC method
    Status ComputeSum(ServerContext* context, ServerReader<NumberRequest>* reader,
                     SumReply* reply) override {
        NumberRequest request;
        int32_t sum = 0;

        // Read each NumberRequest message from the client stream
        while (reader->Read(&request)) {
            std::cout << "Received number: " << request.number() << std::endl;
            sum += request.number();
        }

        // Set the sum in the response
        reply->set_sum(sum);
        std::cout << "Computed sum: " << sum << std::endl;

        return Status::OK;
    }
};

// Function to run the server
void RunServer() {
    std::string server_address("0.0.0.0:8081"); // Using a different port to avoid conflict
    SumServiceImpl service;

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with clients.
    builder.RegisterService(&service);
    // Assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown.
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}
