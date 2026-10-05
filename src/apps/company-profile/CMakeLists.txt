add_executable(company-profile main.cpp)

target_link_libraries(company-profile PRIVATE 
    Qt6::Core 
    Qt6::Sql
)

target_include_directories(company-profile PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../azzammar-core)

