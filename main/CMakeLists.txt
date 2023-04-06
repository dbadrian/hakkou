idf_component_register(
    SRCS
        "main.cpp"
        "platform/platform_esp32.cpp"
        "logger.cpp"
        "event.cpp"
        "shared.cpp"
        "controller.cpp"
        "hardware/fan.cpp"
        "hardware/lcd.cpp"
        "hardware/bme280.cpp"
        "hardware/h_ds18x20.cpp"
        "hardware/pid.cpp"
        "hardware/heater.cpp"
        "hardware/nec_remote.cpp"
        # external sources
        "ext/nec_ir_remote.cpp"
        #####
        # "preferences.cpp"


    INCLUDE_DIRS "."
)

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../www/")
if(EXISTS ${WEB_SRC_DIR})
    spiffs_create_partition_image(www ${WEB_SRC_DIR} FLASH_IN_PROJECT)
    message(STATUS "COPIED WEB DATA!!!!!")

else()
    message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit.")
endif()

idf_build_set_property(COMPILE_OPTIONS "-O2" APPEND)

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
