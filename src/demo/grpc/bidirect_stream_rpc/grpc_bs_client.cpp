// grpc_bs_client.cpp

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <mutex>

#include <grpcpp/grpcpp.h>
#include "chat.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReaderWriter;
using chat::ChatService;
using chat::ChatMessage;

// ChatClient handles communication with the ChatService
class ChatClient {
public:
    ChatClient(std::shared_ptr<Channel> channel)
        : stub_(ChatService::NewStub(channel)) {}

    // Starts the chat session
    void Chat() {
        ClientContext context;
        std::shared_ptr<ClientReaderWriter<ChatMessage, ChatMessage>> stream(
            stub_->Chat(&context));

        // Mutex to synchronize console output
        std::mutex mu_;

        // Thread to read messages from the server
        std::thread writer([stream, &mu_]() {
            std::string input;
            std::string user = "Client";

            std::cout << "You can start typing messages. Type 'exit' to quit.\n";

            while (true) {
                std::getline(std::cin, input);
                if (input == "exit") {
                    stream->WritesDone();
                    break;
                }

                ChatMessage msg;
                msg.set_user(user);
                msg.set_message(input);

                {
                    std::lock_guard<std::mutex> lock(mu_);
                    std::cout << "You: " << input << std::endl;
                }

                if (!stream->Write(msg)) {
                    std::cerr << "Failed to write to server stream." << std::endl;
                    break;
                }
            }
        });

        // Main thread to read messages from the server
        ChatMessage server_msg;
        while (stream->Read(&server_msg)) {
            std::lock_guard<std::mutex> lock(mu_);
            std::cout << server_msg.user() << ": " << server_msg.message() << std::endl;
        }

        writer.join();

        Status status = stream->Finish();
        if (!status.ok()) {
            std::cerr << "Chat RPC failed: " << status.error_message() << std::endl;
        }
    }

private:
    std::unique_ptr<ChatService::Stub> stub_;
};

// Function to display help message
void ShowHelp() {
    std::cout << "Usage:\n";
    std::cout << "  grpc_bs_client -h                Show this help message.\n";
    std::cout << "  grpc_bs_client -u <uri>          Connect to <uri> and start chat.\n";
    std::cout << "\nExample:\n";
    std::cout << "  grpc_bs_client -u localhost:8081\n";
    std::cout << "  (This connects to the server at localhost on port 8081)\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Insufficient arguments.\n";
        ShowHelp();
        return 1;
    }

    std::string option = argv[1];
    if (option == "-h") {
        ShowHelp();
        return 0;
    }
    else if (option == "-u") {
        if (argc != 3) {
            std::cerr << "URI not provided.\n";
            ShowHelp();
            return 1;
        }

        std::string uri = argv[2];

        // Instantiate the client. It requires a channel, out of which the actual RPCs are created.
        ChatClient client(grpc::CreateChannel(
            uri, grpc::InsecureChannelCredentials()));
        client.Chat();
    }
    else {
        std::cerr << "Unknown option: " << option << "\n";
        ShowHelp();
        return 1;
    }

    return 0;
}
