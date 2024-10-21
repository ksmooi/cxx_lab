# Introduction to the gRPC Unary Communication Project

## Overview
This project demonstrates a simple gRPC-based client-server model using unary communication. Unary communication refers to a one-to-one interaction where the client sends a single request to the server, and the server responds with a single reply. The project involves two key components: a gRPC server that provides the "Greeter" service and a gRPC client that communicates with this server to request and receive a greeting message. Both client and server use protocol buffer messages as defined in the `hello.proto` file.

## gRPC Server Implementation
The gRPC server in this project implements a `GreeterServiceImpl` class, which overrides the `SayHello` method from the Greeter service defined in the protocol buffer. The server listens on a specific address and handles incoming client requests by returning a greeting message. The key features of the server include:

- **SayHello Method**: This method accepts a `HelloRequest` containing the name of the client and responds with a `HelloReply` message that includes a personalized greeting.
- **Server Setup**: The server uses `grpc::ServerBuilder` to set up a listening port at `0.0.0.0:8081` and registers the `GreeterServiceImpl` service. The server operates without authentication, using `InsecureServerCredentials()` for simplicity.

The server logs the names of the clients from which it receives requests, showcasing the interaction.

## gRPC Client Implementation
On the client side, the `GreeterClient` class is responsible for communicating with the server. It creates a gRPC channel to connect to the server and invokes the `SayHello` method. The client prompts the user to input their name, which is then sent to the server. Upon receiving the server's reply, it prints the greeting message.

- **SayHello Method**: The client constructs a `HelloRequest` with the user’s input and sends it to the server. The response, in the form of a `HelloReply`, contains the greeting message, which the client displays.
- **Client Context**: A `grpc::ClientContext` is used to manage the RPC lifecycle and can be extended to pass additional metadata or control aspects of the request.
- **Channel Creation**: The client connects to the server using a non-authenticated channel created with `InsecureChannelCredentials()`.

The client utilizes the Boost Program Options library to allow users to specify the server's URI at runtime.

## Protocol Buffer Definitions
The interaction between the client and server is defined using the `hello.proto` file. This file includes two key messages:
- **HelloRequest**: Contains the client’s name, which the server uses to generate a personalized greeting.
- **HelloReply**: Contains the greeting message that the server sends back to the client.

The `Greeter` service within the proto file defines the RPC method `SayHello`, which both the server and client rely on to exchange information.

In summary, this project showcases the basic usage of gRPC for building a simple client-server application with unary communication. It demonstrates how protocol buffers, gRPC channels, and client-server interaction can be implemented to enable efficient communication between systems.