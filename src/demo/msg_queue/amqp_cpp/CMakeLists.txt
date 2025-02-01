add_executable(amqpcpp_queue_server amqpcpp_queue_server.cpp)
target_link_libraries(amqpcpp_queue_server PRIVATE 
    boost_system 
    boost_program_options 
    boost_thread
    ssl 
    crypto 
    amqpcpp
)

add_executable(amqpcpp_queue_client amqpcpp_queue_client.cpp)
target_link_libraries(amqpcpp_queue_client PRIVATE 
    boost_system 
    boost_program_options 
    boost_thread
    ssl 
    crypto 
    amqpcpp
)

add_executable(amqpcpp_pubsub_publisher amqpcpp_pubsub_publisher.cpp)
target_link_libraries(amqpcpp_pubsub_publisher PRIVATE 
    boost_system 
    boost_program_options 
    boost_thread
    ssl 
    crypto 
    amqpcpp
)

add_executable(amqpcpp_pubsub_subscriber amqpcpp_pubsub_subscriber.cpp)
target_link_libraries(amqpcpp_pubsub_subscriber PRIVATE 
    boost_system 
    boost_program_options 
    boost_thread
    ssl 
    crypto 
    amqpcpp
)

add_executable(demo_async_amqp_exchange demo_async_amqp_exchange.cpp)
target_link_libraries(demo_async_amqp_exchange PRIVATE 
    boost_system 
    boost_program_options 
    boost_thread
    ssl 
    crypto 
    amqpcpp
)

add_executable(demo_async_amqp_queue demo_async_amqp_queue.cpp)
target_link_libraries(demo_async_amqp_queue PRIVATE 
    boost_system 
    boost_program_options 
    boost_thread
    ssl 
    crypto 
    amqpcpp
)

add_executable(demo_async_amqp_get_ack demo_async_amqp_get_ack.cpp)
target_link_libraries(demo_async_amqp_get_ack PRIVATE 
    boost_system 
    boost_program_options 
    boost_thread
    ssl 
    crypto 
    amqpcpp
)

add_executable(demo_async_amqp_channel demo_async_amqp_channel.cpp)
target_link_libraries(demo_async_amqp_channel PRIVATE 
    boost_system 
    boost_program_options 
    boost_thread
    ssl 
    crypto 
    amqpcpp
)

add_executable(demo_async_amqp_transaction demo_async_amqp_transaction.cpp)
target_link_libraries(demo_async_amqp_transaction PRIVATE 
    boost_system 
    boost_program_options 
    boost_thread
    ssl 
    crypto 
    amqpcpp
)
