cmake_minimum_required(VERSION 3.15)

project(app_sound)


aux_source_directory(. SOURCES)

add_executable(app_sound ${SOURCES})

# 包含 SpeexDSP 的头文件
target_include_directories(app_sound PRIVATE 
    /home/xiaoyubb/xiangmu/app_sdk_v2/third_party/install/speexdsp/include
)

# 包含库文件目录
target_link_directories(app_sound PRIVATE 
    /home/xiaoyubb/xiangmu/app_sdk_v2/third_party/install/speexdsp/lib
)

# 链接 SpeexDSP 库 (通常叫 speexdsp)
target_link_libraries(app_sound PRIVATE 
    speexdsp
)

target_include_directories(app_sound PRIVATE
                                ./
                                ${CMAKE_SOURCE_DIR}/component/font
                                ${CMAKE_SOURCE_DIR}/lvgl
                                ${CMAKE_SOURCE_DIR}/lvgl/demos
                                ${CMAKE_SOURCE_DIR}/component/osal
                                ${CMAKE_SOURCE_DIR}/component/player
                                )


target_link_libraries(app_sound PRIVATE 
                           lvgl_demos
                           lvgl
                           font
                           player
                           osal
                           asound 
                           pthread 
                           opus 
                           speexdsp
                           )



