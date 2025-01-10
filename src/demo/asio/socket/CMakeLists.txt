add_executable(socket_read_write socket_read_write.cpp)

add_executable(socket_tcp_server socket_tcp_server.cpp)

add_executable(socket_tcp_client socket_tcp_client.cpp)

add_executable(socket_ssl_server socket_ssl_server.cpp)
target_link_libraries(socket_ssl_server PRIVATE boost_system ssl crypto)

add_executable(socket_ssl_client socket_ssl_client.cpp)
target_link_libraries(socket_ssl_client PRIVATE boost_system ssl crypto)

add_executable(uws_ws_ssl_server uws_ws_ssl_server.cpp)
target_link_libraries(uws_ws_ssl_server PRIVATE boost_system ssl crypto z -l:uSockets.a)
target_include_directories(uws_ws_ssl_server PRIVATE /usr/local/include/uWebSockets)

add_executable(beast_ws_ssl_client beast_ws_ssl_client.cpp)
target_link_libraries(beast_ws_ssl_client PRIVATE boost_system ssl crypto z)

