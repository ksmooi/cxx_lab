add_executable(sensor_monitor main_app.cpp sensor.hpp sensor_manager.hpp)
target_link_libraries(sensor_monitor PRIVATE Boost::system Boost::thread)
