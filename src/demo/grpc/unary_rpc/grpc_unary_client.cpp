#include <grpcpp/grpcpp.h>
#include "hello.grpc.pb.h"

#include <boost/program_options.hpp>

#include <iostream>
#include <memory>
#include <string>

namespace po = boost::program_options;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using hello::Greeter;
using hello::HelloRequest;
using hello::HelloReply;

// GreeterClient handles communication with the Greeter service
class GreeterClient {
public:
    GreeterClient(std::shared_ptr<Channel> channel)
        : stub_(Greeter::NewStub(channel)) {}

    // Sends a HelloRequest to the server and prints the HelloReply.
    void SayHello(const std::string& user) {
        // Data we are sending to the server.
        HelloRequest request;
        request.set_name(user);

        // Container for the data we expect from the server.
        HelloReply reply;

        // Context for the client. It could be used to convey extra information to the server and/or tweak certain RPC behaviors.
        ClientContext context;

        // The actual RPC.
        Status status = stub_->SayHello(&context, request, &reply);

        // Act upon its status.
        if (status.ok()) {
            std::cout << "Greeter received: " << reply.message() << std::endl;
        }
        else {
            std::cout << "RPC failed: " << status.error_message() << std::endl;
        }
    }

private:
    std::unique_ptr<Greeter::Stub> stub_;
};

int main(int argc, char** argv) {
    // Define and parse the program options
    std::string uri;
    bool show_help = false;

    try {
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "Display help message")
            ("uri,u", po::value<std::string>()->required(), "Set server URI");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            return 0;
        }

        if (vm.count("uri")) {
            uri = vm["uri"].as<std::string>();
        }
    }
    catch (std::exception& e) {
        std::cerr << "Error parsing command line: " << e.what() << "\n";
        return 1;
    }

    // Instantiate the client. It requires a channel, out of which the actual RPCs are created.
    // We indicate that the channel isn't authenticated (use of InsecureChannelCredentials()).
    GreeterClient greeter(grpc::CreateChannel(uri, grpc::InsecureChannelCredentials()));
    std::string name;
    std::cout << "Enter your name: ";
    std::cin >> name;
    greeter.SayHello(name);
    return 0;
}
