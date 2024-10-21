// grpc_ss_server.cpp

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

#include <grpcpp/grpcpp.h>
#include "streaming.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using streaming::NumberGenerator;
using streaming::NumberRequest;
using streaming::NumberResponse;

// Logic and data behind the server's behavior.
class NumberGeneratorServiceImpl final : public NumberGenerator::Service {
public:
    // Override the GenerateNumbers RPC method
    Status GenerateNumbers(ServerContext* context, const NumberRequest* request,
                           ServerWriter<NumberResponse>* writer) override {
        int32_t start = request->start();
        int32_t count = request->count();

        std::cout << "Received GenerateNumbers request: start=" << start
                  << ", count=" << count << std::endl;

        for (int32_t i = 0; i < count; ++i) {
            // Check if the client has canceled the request
            if (context->IsCancelled()) {
                std::cout << "Client canceled the request." << std::endl;
                return Status::CANCELLED;
            }

            NumberResponse response;
            response.set_number(start + i);
            if (!writer->Write(response)) {
                // Broken stream
                std::cout << "Failed to write to client. Stream might be broken." << std::endl;
                return Status::CANCELLED;
            }

            std::cout << "Sent number: " << response.number() << std::endl;

            // Simulate some delay
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }

        std::cout << "Finished streaming numbers." << std::endl;
        return Status::OK;
    }
};

// Function to run the server
void RunServer() {
    std::string server_address("0.0.0.0:8081");
    NumberGeneratorServiceImpl service;

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
