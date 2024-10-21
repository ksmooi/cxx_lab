# Introduction to the gRPC Bidirectional Streaming Project

## Overview
This project demonstrates a gRPC bidirectional streaming communication model where both the client and the server can send a stream of messages to each other concurrently. It includes a chat service where both the server and client exchange `ChatMessage` objects in real-time, allowing for a dynamic, interactive communication. The project consists of a gRPC server that listens for client messages and responds to them, and a client that sends messages and receives replies from the server.

## gRPC Server Implementation
The server side of the project is implemented in the `grpc_bs_server.cpp` file, which defines the `ChatServiceImpl` class responsible for handling client-server chat interactions.

- **Chat Method**: The `Chat` method in the server overrides the corresponding gRPC method to handle bidirectional communication. The server reads messages from the client using the `ServerReaderWriter` interface. For every client message received, the server responds with a confirmation message containing the text `"Received: "` followed by the original message from the client. The communication continues until the client closes the stream.
- **Thread-Safe Logging**: The server uses a mutex to synchronize console output, ensuring that messages from multiple threads do not interfere with each other. This allows the server to log each received message and its response safely.
- **Server Setup**: The server listens on the address `0.0.0.0:8081` and uses `InsecureServerCredentials()` to allow communication without authentication. The server logs all interactions and runs continuously, awaiting client messages.

## gRPC Client Implementation
The client implementation in the `grpc_bs_client.cpp` file defines a `ChatClient` class, which facilitates communication with the server.

- **Chat Method**: The `Chat` method establishes a bidirectional stream with the server using the `ClientReaderWriter` interface. The client sends messages from the user to the server and receives responses concurrently. The client reads the user’s input in a loop and sends each message to the server. If the user types `"exit"`, the client signals the end of the message stream.
- **Multithreading for Bidirectional Streaming**: The client implements bidirectional streaming by using two threads: one for sending messages and another for receiving server responses. This setup ensures that the client can send and receive messages simultaneously.
- **Client Setup**: The client connects to the server using a URI provided by the user. The communication occurs via a gRPC channel created with `InsecureChannelCredentials()`. The client logs the messages sent and received to provide a real-time chat experience.

## Protocol Buffer Definitions
The communication between the client and server is facilitated by the `chat.proto` file, which defines the structure of the messages and the gRPC service:

- **ChatMessage**: This message includes the user name and the content of the message. It allows both the client and server to exchange text messages during the chat session.
- **ChatService**: The gRPC service that defines the `Chat` method, which enables bidirectional streaming of `ChatMessage` objects between the client and server.

In summary, this project demonstrates a gRPC bidirectional streaming communication model, providing a real-time chat service where both the client and server can send and receive messages concurrently. This setup is a practical example of how gRPC can be used for interactive, low-latency communication in applications such as chat systems.

