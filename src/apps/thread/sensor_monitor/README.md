# Sensor Monitoring System in C++ Using Boost.Asio

## Overview of the Project

This project implements a **Sensor Monitoring System** in C++ using the **Boost.Asio** library for asynchronous operations and threading. The system allows multiple sensors to be managed concurrently, with each sensor simulating data acquisition (e.g., temperature and humidity). These sensors operate asynchronously and can be dynamically added, started, or stopped by a **SensorManager**. The project emphasizes efficient multithreading and real-time sensor data acquisition, which makes it highly scalable and performant for environments where concurrent sensor data processing is required.

## Key Components

### SensorManager: Managing Multiple Sensors and Threading

The `SensorManager` class is responsible for handling multiple sensors and managing their lifecycle, such as starting, stopping, and running them concurrently in different threads【166†source】.

- **Multithreading**: The `SensorManager` uses `boost::asio::io_context` to manage asynchronous operations for each sensor. It spawns multiple threads, leveraging `boost::thread::hardware_concurrency()` to determine the optimal number of threads based on available hardware【166†source】.
- **Sensor Management**: The manager supports adding sensors dynamically, allowing for flexible management of different sensor types. Once sensors are added, the manager starts all sensors and ensures they run in separate threads.

This centralized management ensures that the system scales efficiently, handling multiple sensors without performance degradation.

### SensorBase and Sensor: Abstracting and Implementing Sensors

The project defines an abstract base class `SensorBase`, which is the foundation for all sensor types, and `Sensor`, a templated class that handles generic sensor operations【165†source】. Each sensor class inherits from these and implements its own data generation logic.

- **TemperatureSensor** and **HumiditySensor** are concrete implementations that generate simulated data using random number generators【165†source】. For example, the `TemperatureSensor` generates temperature values within a certain range, while the `HumiditySensor` produces random humidity levels.
- **Asynchronous Operations**: Sensors use `boost::asio::steady_timer` to schedule periodic data generation, ensuring that data acquisition happens at regular intervals without blocking other operations【165†source】.

This design allows for easy expansion by adding new sensor types without altering the core functionality of the system.

### Main Application: Starting and Managing Sensors

The main application (`main_app.cpp`) is the entry point of the system【164†source】. It initializes the `SensorManager`, creates multiple sensors (temperature and humidity), and connects them to output slots that display the sensor data. The system runs continuously, with real-time data acquisition from the sensors until a shutdown signal (`Ctrl+C`) is received.

- **Signal Handling**: The system captures `SIGINT` to gracefully stop all sensors and shut down the system【164†source】. When the shutdown signal is received, the `SensorManager` stops all sensors and joins all threads.
- **Sensor Data Display**: Each sensor connects its data signals to slots that print the data to the console. For instance, temperature and humidity readings are printed every second as they are generated.

### Build and Configuration

The project uses **CMake** to configure and build the application【167†source】. The `CMakeLists.txt` file ensures that the required Boost libraries (`Boost::system` and `Boost::thread`) are linked during compilation. This setup simplifies the build process, making it easy to compile and run the application on any system with Boost installed.

```bash
# Build the project
mkdir build
cd build
cmake ..
make

# Run the application
./sensor_monitor
```

## Conclusion

This Sensor Monitoring System provides a robust framework for handling multiple sensors in real-time using asynchronous programming with Boost.Asio. The project demonstrates how to build scalable and efficient systems in C++ that can manage dynamic tasks, such as sensor data acquisition, in a multithreaded environment. By leveraging the flexibility of `SensorManager`, new sensor types can be added with minimal effort, making this project an excellent foundation for any real-time data monitoring application.