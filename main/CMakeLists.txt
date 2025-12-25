idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES arduino_esp32 grbl_esp32s3 spiffs)

add_compile_options(-Wno-unused-variable)