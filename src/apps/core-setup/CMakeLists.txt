add_executable(core-setup main.cpp)

# Link ke library internal/common/core jika nanti dibutuhkan, serta Qt
target_link_libraries(core-setup PRIVATE 
    Qt6::Core 
    Qt6::Sql
)

target_include_directories(core-setup PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../azzammar-core)

