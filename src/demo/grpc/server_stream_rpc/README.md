# Introduction to the gRPC Server-Streaming Project

## Overview
This project showcases a gRPC server-streaming communication model where the server streams multiple responses to a single client request. It demonstrates how to implement a gRPC service that continuously sends data (in this case, numbers) to a client over a single request. The project includes a gRPC server that generates a sequence of numbers and streams them to the client, as well as a client that requests and processes these streamed numbers. Both components utilize protocol buffer messages defined in the `streaming.proto` file.

## gRPC Server Implementation
The gRPC server is implemented in the `grpc_ss_server.cpp` file. The server provides a service called `NumberGeneratorServiceImpl` which implements the `GenerateNumbers` method to handle incoming requests and stream a sequence of numbers to the client. Below are the key aspects of the server's functionality:

- **GenerateNumbers Method**: The server receives a `NumberRequest` containing the starting number and the count of numbers to generate. It then streams the requested sequence of numbers to the client in multiple `NumberResponse` messages.
- **Server Streaming**: The method uses the `ServerWriter` interface to send multiple `NumberResponse` messages over time. The server ensures a continuous stream of data until all numbers have been sent or the client cancels the request.
- **Concurrency Handling**: The server checks whether the client cancels the request during the streaming process and stops the transmission accordingly. This ensures robust communication and efficient resource management.
- **Server Setup**: The server listens on the address `0.0.0.0:8081` using `InsecureServerCredentials()` for simplicity. It logs details of the client requests, providing insight into the streamed data.

## gRPC Client Implementation
On the client side, the project uses a `NumberGeneratorClient` class, as defined in `grpc_ss_client.cpp`, to communicate with the server and handle streamed responses. Key features of the client implementation include:

- **GenerateNumbers Method**: The client sends a `NumberRequest` to the server, specifying the starting number and the count of numbers to receive. It then processes the stream of `NumberResponse` messages containing the generated numbers.
- **Client-Side Streaming**: Using the `ClientReader` interface, the client reads and prints each number streamed by the server in real-time. The client logs each number it receives and checks the status of the communication after the streaming is complete.
- **Client Setup**: The client connects to the server through a non-authenticated channel, specified by the user at runtime. The connection is established using `InsecureChannelCredentials()` for simplicity, allowing the client to communicate without advanced security features.

## Protocol Buffer Definitions
The communication between the client and server is defined using protocol buffer messages in the `streaming.proto` file. This file outlines the structure of the messages and the service used in the gRPC project:

- **NumberRequest**: Contains the starting number and the count of numbers that the client requests from the server.
- **NumberResponse**: Contains a single number that is sent as part of the stream from the server to the client.
- **NumberGenerator Service**: Defines the RPC method `GenerateNumbers`, which allows the server to stream multiple `NumberResponse` messages in response to a single `NumberRequest` from the client.

This project demonstrates the fundamental principles of gRPC server-streaming communication, where a server can send a sequence of responses over time while the client processes the stream. It provides a practical use case for building real-time data streaming applications using gRPC.

