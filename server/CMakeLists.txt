add_executable(azzammar_server
    src/main.cpp
)

# The server requires Qt6 Network alongside the Core library
target_link_libraries(azzammar_server PRIVATE azzammar_core Qt6::Core Qt6::Network)
