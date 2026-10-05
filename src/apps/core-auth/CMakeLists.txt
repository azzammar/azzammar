add_executable(core-auth main.cpp)

# Menggunakan Network dan NetworkAuth untuk gerbang Google API OAuth
target_link_libraries(core-auth PRIVATE 
    Qt6::Core 
    Qt6::Network 
    Qt6::NetworkAuth
    Qt6::Sql
)

target_include_directories(core-auth PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../azzammar-core)

