# Always compiled source files
set(srcs src/ESP32SSDP.cpp )

# Always included headers
set(includes src )

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${includes}
                       REQUIRES arduino_esp32 )