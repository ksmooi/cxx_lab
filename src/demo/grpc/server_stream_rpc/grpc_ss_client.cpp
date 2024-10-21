// grpc_ss_client.cpp

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>
#include "streaming.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientReader;
using streaming::NumberGenerator;
using streaming::NumberRequest;
using streaming::NumberResponse;

// GreeterClient handles communication with the NumberGenerator service
class NumberGeneratorClient {
public:
    NumberGeneratorClient(std::shared_ptr<Channel> channel)
        : stub_(NumberGenerator::NewStub(channel)) {}

    // Sends a NumberRequest to the server and reads the stream of NumberResponses.
    void GenerateNumbers(int32_t start, int32_t count) {
        // Data we are sending to the server.
        NumberRequest request;
        request.set_start(start);
        request.set_count(count);

        // Context for the client. It could be used to convey extra information to the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // Container for the data we expect from the server.
        NumberResponse response;

        // Create a reader for the server stream.
        std::unique_ptr<ClientReader<NumberResponse>> reader(stub_->GenerateNumbers(&context, request));

        std::cout << "Requesting numbers from " << start << " to " << start + count - 1 << std::endl;

        // Read the stream of responses from the server.
        while (reader->Read(&response)) {
            std::cout << "Received number: " << response.number() << std::endl;
        }

        // Check the status after the stream is complete.
        Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "Finished receiving numbers." << std::endl;
        }
        else {
            std::cout << "GenerateNumbers RPC failed: " << status.error_message() << std::endl;
        }
    }

private:
    std::unique_ptr<NumberGenerator::Stub> stub_;
};

// Function to display help message
void ShowHelp() {
    std::cout << "Usage:\n";
    std::cout << "  grpc_ss_client -h                         # Show this help message.\n";
    std::cout << "  grpc_ss_client -u <uri> <start> <count>   # Connect to <uri> and request numbers starting from <start> with <count> numbers.\n";
    std::cout << "\nExample:\n";
    std::cout << "  grpc_ss_client -u localhost:8081 10 5\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Insufficient arguments.\n";
        ShowHelp();
        return 1;
    }

    std::string mode = argv[1];
    if (mode == "-h") {
        ShowHelp();
        return 0;
    }
    else if (mode == "-u") {
        if (argc != 5) {
            std::cerr << "Invalid number of arguments for -u option.\n";
            ShowHelp();
            return 1;
        }

        std::string uri = argv[2];
        int32_t start = std::stoi(argv[3]);
        int32_t count = std::stoi(argv[4]);

        // Instantiate the client. It requires a channel, out of which the actual RPCs are created.
        NumberGeneratorClient client(grpc::CreateChannel(
            uri, grpc::InsecureChannelCredentials()));
        client.GenerateNumbers(start, count);
    }
    else {
        std::cerr << "Unknown option: " << mode << "\n";
        ShowHelp();
        return 1;
    }

    return 0;
}
