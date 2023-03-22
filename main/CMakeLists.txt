idf_component_register(SRCS 
                        "main.cpp"
                        "event.cpp"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES freertos
                    )


idf_build_set_property(COMPILE_OPTIONS "-O2" APPEND)

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
