idf_component_register(
    SRCS
        "main.cpp"
        "logger.cpp"
        "event.cpp"
        "preferences.cpp"
        # "controller.cpp"
        "hardware/fan.cpp"
        "hardware/lcd.cpp"
        "hardware/bme280.cpp"
        "hardware/h_ds18x20.cpp"
        "hardware/pid.cpp"
        "hardware/heater.cpp"
        "platform/platform_esp32.cpp"
    INCLUDE_DIRS "."
)


idf_build_set_property(COMPILE_OPTIONS "-O2" APPEND)

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
