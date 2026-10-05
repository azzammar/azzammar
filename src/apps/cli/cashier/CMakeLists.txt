add_executable(cashier_cli
    src/main.cpp
)

# Direct dependencies: The Core library and Qt6 Core
target_link_libraries(cashier_cli PRIVATE azzammar_core Qt6::Core)

