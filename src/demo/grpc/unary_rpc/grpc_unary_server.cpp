#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "hello.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using hello::Greeter;
using hello::HelloRequest;
using hello::HelloReply;

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
public:
    // Override the SayHello RPC method
    Status SayHello(ServerContext* context, const HelloRequest* request,
                   HelloReply* reply) override {
        std::string prefix("Hello ");
        reply->set_message(prefix + request->name());
        std::cout << "Received request from: " << request->name() << std::endl;
        return Status::OK;
    }
};

// Function to run the server
void RunServer() {
    std::string server_address("0.0.0.0:8081");
    GreeterServiceImpl service;

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
