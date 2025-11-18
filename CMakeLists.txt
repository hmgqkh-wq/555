cmake_minimum_required(VERSION 3.10)
project(vulkan_xeno_wrapper C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Build a shared library named libvulkan.so (this is important for Eden)
add_library(vulkan SHARED
    src/wrapper.c
)

# Ensure the output file is libvulkan.so (no version suffix)
set_target_properties(vulkan PROPERTIES
    OUTPUT_NAME "vulkan"
    PREFIX "lib"
)

# Link to dl for dlsym/dlopen on Android host
find_library(LIB_DL dl)
if(LIB_DL)
    target_link_libraries(vulkan PRIVATE ${LIB_DL})
endif()

# install rules to stage files into package root
install(TARGETS vulkan
    LIBRARY DESTINATION .
)

# Also install config files
install(DIRECTORY config/ DESTINATION config)
install(FILES manifest.json DESTINATION .)
