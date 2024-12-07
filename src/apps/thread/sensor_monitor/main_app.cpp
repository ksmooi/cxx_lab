#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>
#include "sensor_manager.hpp"

std::atomic<bool> running(true);
std::shared_ptr<SensorManager> sensor_manager;

int main() {
    // Set up signal handler
    std::signal(SIGINT, [](int signal) {
        if (signal == SIGINT) {
            std::cout << "\nCtrl-C pressed. Shutting down...\n";
            running = false;
            if (sensor_manager) {
                sensor_manager->stop_all();
            }
        }
    });

    // Create SensorManager
    sensor_manager = std::make_shared<SensorManager>();

    // Create sensors
    auto temp_sensor = std::make_shared<TemperatureSensor>(sensor_manager->get_io_context());
    auto humidity_sensor = std::make_shared<HumiditySensor>(sensor_manager->get_io_context());

    // Connect sensor signals to slots
    temp_sensor->connect([](const double& temp) {
        std::cout << "[TemperatureSensor] New Temperature: " << temp << " °C" << std::endl;
    });

    humidity_sensor->connect([](const int& humidity) {
        std::cout << "[HumiditySensor] New Humidity: " << humidity << " %" << std::endl;
    });

    // Add sensors to manager
    sensor_manager->add_sensor(temp_sensor);
    sensor_manager->add_sensor(humidity_sensor);

    // Start all sensors
    sensor_manager->start_all();

    std::cout << "Sensor Monitoring System started. Press Ctrl-C to stop.\n";

    // Main loop
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Sensor Monitoring System stopped." << std::endl;

    return 0;
}
