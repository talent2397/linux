cmake_minimum_required(VERSION 3.15)

project(app_control_center)


aux_source_directory(. SOURCES)

add_executable(app_control_center ${SOURCES})


# 1. 告诉编译器去哪里找 OpenSSL 的头文件 (<openssl/conf.h> 等)
target_include_directories(app_control_center PRIVATE 
    /home/xiaoyubb/xiangmu/app_sdk_v2/third_party/install/openssl/include
)

# 2. 告诉链接器去哪里找 OpenSSL 的库文件 (libssl.so 和 libcrypto.so)
target_link_directories(app_control_center PRIVATE 
    /home/xiaoyubb/xiangmu/app_sdk_v2/third_party/install/openssl/lib
)

# 3. 链接这两个库 (注意：如果你的工程本来就有 target_link_libraries，直接把 ssl 和 crypto 加进去就行)
target_link_libraries(app_control_center PRIVATE 
    ssl 
    crypto
)
# 告诉链接器去哪里找库文件
target_link_directories(app_control_center PRIVATE 
    /home/xiaoyubb/xiangmu/app_sdk_v2/third_party/install/openssl/lib
    /home/xiaoyubb/xiangmu/app_sdk_v2/third_party/install/boost/lib    # <--- 新增这一行：告诉链接器 Boost 库在哪里
)
target_include_directories(app_control_center PRIVATE
                                ./
                                ${CMAKE_SOURCE_DIR}/component/font
                                ${CMAKE_SOURCE_DIR}/lvgl
                                ${CMAKE_SOURCE_DIR}/lvgl/demos
                                ${CMAKE_SOURCE_DIR}/component/osal
                                ${CMAKE_SOURCE_DIR}/component/player
                                )


target_link_libraries(app_control_center PRIVATE 
                           lvgl_demos
                           lvgl
                           font
                           player
                           osal
                           asound 
                           pthread 
                           opus 
                           curl
                           boost_system
                           ssl
                           crypto
                           nghttp2
                           )



