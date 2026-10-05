add_executable(azzammar_cli
    src/main.cpp
)

target_link_libraries(azzammar_cli PRIVATE azzammar_core Qt6::Core Qt6::Widgets Qt6::Gui Qt6::WebEngineWidgets)
