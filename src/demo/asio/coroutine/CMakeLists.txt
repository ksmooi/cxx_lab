add_executable(async_blocking_api_with_ioctx async_blocking_api_with_ioctx.cpp)

add_executable(async_blocking_api_with_pool async_blocking_api_with_pool.cpp)

add_executable(demo_compose_coroutine demo_compose_coroutine.cpp)
target_link_libraries(demo_compose_coroutine PRIVATE pthread boost_system boost_program_options)

add_executable(demo_spawn_coroutine demo_spawn_coroutine.cpp)
target_link_libraries(demo_spawn_coroutine PRIVATE pthread boost_system boost_program_options)

