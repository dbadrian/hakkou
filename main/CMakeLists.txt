idf_component_register(
    SRCS
        "main.cpp"
        "logger.cpp"
        "event.cpp"
        "platform_esp32.cpp"
    INCLUDE_DIRS "."
)


idf_build_set_property(COMPILE_OPTIONS "-O2" APPEND)

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
