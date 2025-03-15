#include <sigslot/signal.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

// SignalSlotManager manages all the signals corresponding to different events
class SignalSlotManager {
public:
    // Signal triggered when a user logs in
    sigslot::signal<std::string> UserLoggedIn;

    // Signal triggered when a user logs out
    sigslot::signal<std::string> UserLoggedOut;

    // Signal triggered when a message is received
    sigslot::signal<std::string, std::string> MessageReceived;

    // Singleton pattern to ensure only one instance exists
    static SignalSlotManager& getInstance() {
        static SignalSlotManager instance;
        return instance;
    }

private:
    // Private constructor to prevent multiple instances
    SignalSlotManager() = default;
};

// Logger class listens to various events and logs them
class Logger {
public:
    Logger() {
        // Connect Logger's slots to the corresponding signals
        auto& manager = SignalSlotManager::getInstance();
        manager.UserLoggedIn.connect(&Logger::onUserLoggedIn, this);
        manager.UserLoggedOut.connect(&Logger::onUserLoggedOut, this);
        manager.MessageReceived.connect(&Logger::onMessageReceived, this);
    }

    // Slot to handle UserLoggedIn event
    void onUserLoggedIn(const std::string& username) {
        std::cout << "[Logger] User Logged In: " << username << std::endl;
    }

    // Slot to handle UserLoggedOut event
    void onUserLoggedOut(const std::string& username) {
        std::cout << "[Logger] User Logged Out: " << username << std::endl;
    }

    // Slot to handle MessageReceived event
    void onMessageReceived(const std::string& sender, const std::string& message) {
        std::cout << "[Logger] Message from " << sender << ": " << message << std::endl;
    }
};

// User class represents a chat user who can send and receive messages
class User : public std::enable_shared_from_this<User> {
public:
    User(const std::string& name) : username(name) {
        // Connect User's slot to the MessageReceived signal
        auto& manager = SignalSlotManager::getInstance();
        manager.MessageReceived.connect(&User::onMessageReceived, this);
    }

    // Method to log in to the chat room
    void login() {
        auto& manager = SignalSlotManager::getInstance();
        manager.UserLoggedIn(username);
    }

    // Method to send a message to the chat room
    void sendMessage(const std::string& message) {
        auto& manager = SignalSlotManager::getInstance();
        manager.MessageReceived(username, message);
    }

    // Slot to handle incoming messages
    void onMessageReceived(const std::string& sender, const std::string& message) {
        std::cout << "[" << username << "] Received message from " << sender << ": " << message << std::endl;
    }

    // Method to log out from the chat room
    void logout() {
        auto& manager = SignalSlotManager::getInstance();
        manager.UserLoggedOut(username);
    }

    // Method to receive messages (used by ChatRoom)
    void receiveMessage(const std::string& sender, const std::string& message) {
        // This method can include additional processing if needed
        onMessageReceived(sender, message);
    }

    // Getter for username
    const std::string& getUsername() const { return username; }

private:
    std::string username; // Username of the user
};

// ChatRoom class handles broadcasting messages to all users
class ChatRoom {
public:
    // Method to add a user to the chat room
    void addUser(const std::shared_ptr<User>& user) {
        users_.push_back(user);
    }

    // Slot to handle incoming messages and broadcast them
    void onMessageReceived(const std::string& sender, const std::string& message) {
        std::cout << "[ChatRoom] Broadcasting message from " << sender << ": " << message << std::endl;
        for (const auto& user : users_) {
            if (user->getUsername() != sender) { // Avoid sending the message back to the sender
                user->receiveMessage(sender, message);
            }
        }
    }

private:
    std::vector<std::shared_ptr<User>> users_; // List of users in the chat room
};

int main() {
    // Instantiate the SignalSlotManager
    auto& manager = SignalSlotManager::getInstance();

    // Create and initialize the Logger
    Logger logger;

    // Create the ChatRoom and connect its slot to the MessageReceived signal
    ChatRoom chatRoom;
    manager.MessageReceived.connect(&ChatRoom::onMessageReceived, &chatRoom);

    // Create users
    auto alice = std::make_shared<User>("Alice");
    auto bob = std::make_shared<User>("Bob");
    auto charlie = std::make_shared<User>("Charlie");

    // Add users to the chat room
    chatRoom.addUser(alice);
    chatRoom.addUser(bob);
    chatRoom.addUser(charlie);

    // Users log in
    alice->login();
    bob->login();
    charlie->login();

    std::cout << "----- Chat Session Start -----" << std::endl;

    // Users send messages
    alice->sendMessage("Hello everyone!");
    bob->sendMessage("Hi Alice!");
    charlie->sendMessage("Good to see you both.");

    std::cout << "----- Chat Session End -----" << std::endl;

    // Users log out
    alice->logout();
    bob->logout();
    charlie->logout();

    return 0;
}
