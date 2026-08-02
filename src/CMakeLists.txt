set(WEB_SOURCES
    web/server.cpp
    web/client.cpp
    RequestRouter/loginhandler.cpp
    RequestRouter/datahandler.cpp
    RequestRouter/router.cpp
)
set(BP_SOURCES
    BPY/DirectoryCrawler/DirectoryCrawler.cpp
    BPY/fbptree/fbptree.cpp
)

# libpqxx
set(libpqxx_DIR "/usr/local/lib/cmake/libpqxx")
find_package(libpqxx REQUIRED CONFIG)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(simdjson CONFIG REQUIRED)

#find_package(Boost CONFIG)
find_package(Boost REQUIRED COMPONENTS random CONFIG)

#add_executable(Ws_server
  #  server_main.cpp
 #   ${WEB_SOURCES}
#)

#add_executable(Ws_client
#    client_main.cpp
#    ${WEB_SOURCES}
#)
add_executable(test
    main.cpp
    ${WEB_SOURCES}
    ${BP_SOURCES}
)
#target_include_directories(Ws_server PRIVATE
#
#    ${CMAKE_CURRENT_SOURCE_DIR}/include
#    ${CMAKE_CURRENT_SOURCE_DIR}/include/BPY
 #   ${CMAKE_CURRENT_SOURCE_DIR}/include/extern
#)

#target_include_directories(Ws_client PRIVATE
#    ${CMAKE_CURRENT_SOURCE_DIR}/include
#    ${CMAKE_CURRENT_SOURCE_DIR}/include/BPY
 #   ${CMAKE_CURRENT_SOURCE_DIR}/include/extern
#)

target_include_directories(test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/extern  
)

#target_link_libraries(Ws_server
#    PRIVATE
    #libpqxx::pqxx
    #OpenSSL::SSL
     #   OpenSSL::Crypto
      #  Boost::random
#)

#target_link_libraries(Ws_client
 #   PRIVATE
  #      libpqxx::pqxx
   #    OpenSSL::SSL
    #    OpenSSL::Crypto
     #   Boost::random
#)

target_link_libraries(test
    PRIVATE 
        libpqxx::pqxx
        OpenSSL::SSL
        OpenSSL::Crypto
        Boost::random    
)

#target_compile_options(Ws_server PRIVATE -O2 -Wall -Wextra -Wno-mismatched-new-delete)
#target_compile_options(Ws_client PRIVATE -O2 -Wall -Wextra -Wno-mismatched-new-delete)
target_compile_options(test PRIVATE -std=c++2c -O3 -Wall -Wextra -Wno-mismatched-new-delete)

install(TARGETS test DESTINATION bin)
