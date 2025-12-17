# GxEPD2 component registration for ESP-IDF
# Registers GxEPD2 sources and declares dependency on Adafruit_GFX

cmake_minimum_required(VERSION 3.5)

# Collect source files: main wrapper + all driver implementations
file(GLOB GXEPD2_SRCS "GxEPD2_EPD.cpp" "epd/*.cpp" "gdey/*.cpp" "extras/*.cpp" "bitmaps/*.c")

idf_component_register(
    SRCS ${GXEPD2_SRCS}
    INCLUDE_DIRS "." "epd" "gdey" "extras" "bitmaps"
    REQUIRES AdafruitGFX
)

project(GxEPD2)

