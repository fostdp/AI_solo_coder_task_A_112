set(BACKEND_SOURCES
    src/main.cpp
    src/fading_model.cpp
    src/mold_model.cpp
    src/clickhouse_client.cpp
    src/alert_engine.cpp
    src/notification.cpp
    src/opcua_client.cpp
    src/http_server.cpp
    src/opcua/session_pool.cpp
    src/ml/response_surface.cpp
)

add_executable(haihunhou_server ${BACKEND_SOURCES})

target_link_libraries(haihunhou_server
    PRIVATE
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:pthread>
)

if(WIN32)
    target_link_libraries(haihunhou_server PRIVATE ws2_32 mswsock)
endif()

target_compile_definitions(haihunhou_server
    PRIVATE
    FRONTEND_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../frontend"
)

install(TARGETS haihunhou_server RUNTIME DESTINATION bin)
