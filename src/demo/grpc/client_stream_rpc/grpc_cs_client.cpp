#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>
#include "sum.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::ClientWriter;
using sum::SumService;
using sum::NumberRequest;
using sum::SumReply;

// SumClient handles communication with the SumService
class SumClient {
public:
    SumClient(std::shared_ptr<Channel> channel)
        : stub_(SumService::NewStub(channel)) {}

    // Sends a stream of NumberRequest messages and receives a SumReply.
    int32_t ComputeSum(const std::vector<int32_t>& numbers) {
        // Data we are sending to the server.
        SumReply reply;
        ClientContext context;

        // Create a ClientWriter to send a stream of NumberRequest messages
        std::unique_ptr<ClientWriter<NumberRequest>> writer(stub_->ComputeSum(&context, &reply));

        // Send each number in the stream
        for (const auto& number : numbers) {
            NumberRequest request;
            request.set_number(number);
            if (!writer->Write(request)) {
                // Broken stream.
                std::cerr << "Failed to write to server." << std::endl;
                break;
            }
            std::cout << "Sent number: " << number << std::endl;
        }

        // Tell the server we've finished sending
        writer->WritesDone();

        // Receive the response from the server
        Status status = writer->Finish();

        if (status.ok()) {
            std::cout << "Sum received from server: " << reply.sum() << std::endl;
            return reply.sum();
        }
        else {
            std::cerr << "ComputeSum RPC failed: " << status.error_message() << std::endl;
            return 0;
        }
    }

private:
    std::unique_ptr<SumService::Stub> stub_;
};

// Function to display help message
void ShowHelp() {
    std::cout << "Usage:\n";
    std::cout << "  grpc_cs_client -h                Show this help message.\n";
    std::cout << "  grpc_cs_client -u <uri> [numbers...]   Connect to <uri> and send a stream of numbers.\n";
    std::cout << "\nExample:\n";
    std::cout << "  grpc_cs_client -u localhost:8081 10 20 30 40 50\n";
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
        if (argc < 3) {
            std::cerr << "URI not provided.\n";
            ShowHelp();
            return 1;
        }

        std::string uri = argv[2];
        std::vector<int32_t> numbers;

        for (int i = 3; i < argc; ++i) {
            try {
                int32_t num = std::stoi(argv[i]);
                numbers.push_back(num);
            }
            catch (const std::invalid_argument& e) {
                std::cerr << "Invalid number: " << argv[i] << std::endl;
                return 1;
            }
            catch (const std::out_of_range& e) {
                std::cerr << "Number out of range: " << argv[i] << std::endl;
                return 1;
            }
        }

        if (numbers.empty()) {
            std::cerr << "No numbers provided to send.\n";
            ShowHelp();
            return 1;
        }

        // Instantiate the client. It requires a channel, out of which the actual RPCs are created.
        SumClient client(grpc::CreateChannel(
            uri, grpc::InsecureChannelCredentials()));
        int32_t sum = client.ComputeSum(numbers);

        std::cout << "Total Sum: " << sum << std::endl;
    }
    else {
        std::cerr << "Unknown option: " << option << "\n";
        ShowHelp();
        return 1;
    }

    return 0;
}
