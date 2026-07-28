set(WEB_SOURCES
    web/server.cpp
    web/client.cpp
)
# libpqxx
set(libpqxx_DIR "/usr/local/lib/cmake/libpqxx")
find_package(libpqxx REQUIRED CONFIG)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(simdjson CONFIG REQUIRED)
# Boost
find_package(Boost CONFIG)

add_executable(Ws_server
    server_main.cpp
    ${WEB_SOURCES}
)

add_executable(Ws_client
    client_main.cpp
    ${WEB_SOURCES}
)

target_include_directories(Ws_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/extern
)

target_include_directories(Ws_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/extern
)

target_link_libraries(Ws_server
    PRIVATE 
        libpqxx::pqxx
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_link_libraries(Ws_client
    PRIVATE 
        libpqxx::pqxx
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_compile_options(Ws_server PRIVATE -O2 -Wall -Wextra -Wno-mismatched-new-delete)
target_compile_options(Ws_client PRIVATE -O2 -Wall -Wextra -Wno-mismatched-new-delete)

install(TARGETS Ws_server Ws_client DESTINATION bin)
