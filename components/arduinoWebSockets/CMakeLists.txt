# Always compiled source files
set(srcs src/WebSockets.cpp
    src/WebSocketsClient.cpp
    src/WebSocketsServer.cpp
    src/libb64/cencode.c
    src/libb64/cdecode.c 
    src/libsha1/libsha1.c )

# Always included headers
set(includes src)

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${includes}
                       REQUIRES arduino_esp32 app_update )