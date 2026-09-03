add_library(azzammar_core SHARED
    include/azzammar_core.h
    src/azzammar_core.cpp
    include/azzammar_google_service.h
    src/azzammar_google_service.cpp
    include/azzammar_crypto.h
    src/azzammar_crypto.cpp
)

# Link Qt6 Core to our shared library
target_link_libraries(azzammar_core PRIVATE Qt6::Core Qt6::Network Qt6::NetworkAuth Qt6::Sql)

# Expose the include folder so other subprojects can find the headers easily
target_include_directories(azzammar_core PUBLIC include)

