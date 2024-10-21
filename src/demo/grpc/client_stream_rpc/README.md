# Introduction to the gRPC Client-Streaming Project

## Overview
This project illustrates a gRPC client-streaming communication model where a client sends a stream of requests to the server, and the server responds with a single reply. The server accumulates the values sent by the client and returns their sum as a response. The project contains a gRPC server that computes the sum of streamed numbers and a client that sends a sequence of numbers to the server.

## gRPC Server Implementation
The server is implemented in the `grpc_cs_server.cpp` file, providing a service called `SumServiceImpl`. This service implements the `ComputeSum` method to handle client requests.

- **ComputeSum Method**: The server receives a stream of `NumberRequest` messages from the client, each containing a number. As the server reads the stream, it adds up the numbers. Once the stream is completed, the server responds with a `SumReply` that contains the total sum.
- **Server Streaming Logic**: The server uses `ServerReader` to handle the stream of incoming numbers and compute their sum. It logs each number it receives and calculates the cumulative sum.
- **Server Setup**: The server listens on the address `0.0.0.0:8081`, using `InsecureServerCredentials()` for simplicity. It logs the incoming requests and the computed sum before sending the response to the client.

## gRPC Client Implementation
The client is implemented in the `grpc_cs_client.cpp` file, which uses a `SumClient` class to communicate with the server.

- **ComputeSum Method**: The client sends a stream of numbers to the server via `NumberRequest` messages. After streaming all the numbers, the client waits for the server’s response, which contains the total sum of the numbers.
- **Client Streaming Logic**: The client uses `ClientWriter` to send the stream of numbers to the server. It sends each number as part of the stream and then marks the end of the stream using `WritesDone()`.
- **Client Setup**: The client connects to the server using a URI specified at runtime and sends a series of numbers. It logs each number sent and the total sum received from the server.

## Protocol Buffer Definitions
The communication between the client and server is defined using protocol buffer messages in the `sum.proto` file:

- **NumberRequest**: This message contains a single number that is streamed from the client to the server.
- **SumReply**: This message contains the sum of the numbers received by the server.
- **SumService**: Defines the `ComputeSum` RPC method, allowing the server to receive a stream of numbers and respond with their sum.

This project demonstrates the key principles of gRPC client-streaming communication, where the client sends a continuous stream of data to the server, and the server responds with a single summary result. It provides a practical application for calculating the sum of a series of numbers using gRPC client streaming.

