#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <mutex>

#include <grpcpp/grpcpp.h>
#include "chat.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;
using chat::ChatService;
using chat::ChatMessage;

// Logic and data behind the server's behavior.
class ChatServiceImpl final : public ChatService::Service {
public:
    // Override the Chat RPC method
    Status Chat(ServerContext* context,
               ServerReaderWriter<ChatMessage, ChatMessage>* stream) override {
        ChatMessage client_msg;
        while (stream->Read(&client_msg)) {
            std::lock_guard<std::mutex> lock(mu_);
            std::cout << client_msg.user() << ": " << client_msg.message() << std::endl;

            // Create a response message
            ChatMessage server_msg;
            server_msg.set_user("Server");
            server_msg.set_message("Received: " + client_msg.message());

            // Send the response back to the client
            if (!stream->Write(server_msg)) {
                std::cerr << "Failed to write to client stream." << std::endl;
                break;
            }
        }
        std::cout << "Chat ended by client." << std::endl;
        return Status::OK;
    }

private:
    std::mutex mu_; // To synchronize console output
};

// Function to run the server
void RunServer() {
    std::string server_address("0.0.0.0:8081"); // Using a distinct port
    ChatServiceImpl service;

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
