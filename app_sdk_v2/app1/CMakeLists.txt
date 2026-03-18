cmake_minimum_required(VERSION 3.15)
# 工程名称demo1
project(demo1)

if(SIMULATOR_LINUX)
    add_compile_definitions(SIMULATOR_LINUX)  
endif()
# 拷贝res文件夹到build中，方便后续打包
file(COPY res DESTINATION ${PROJECT_BINARY_DIR})

# 编译当前目录下所有.c文件
aux_source_directory(. SOURCES)
aux_source_directory(ui SOURCES)
# 生成名称为app1的可执行文件
add_executable(demo1 ${SOURCES})
# 引入头文件路径
target_include_directories(demo1 PRIVATE
                                ./
                                ./res
                                ./res/font/
                                ./res/image/
                                ./res/music/
                                ./ui
                                ${CMAKE_SOURCE_DIR}/component/font
                                ${CMAKE_SOURCE_DIR}/lvgl
                                ${CMAKE_SOURCE_DIR}/component/wifi
                                ${CMAKE_SOURCE_DIR}/component/net
                                ${CMAKE_SOURCE_DIR}/component/hal
                                ${CMAKE_SOURCE_DIR}/component/player
                                )
# 引入依赖库
target_link_libraries(demo1 PRIVATE
                        lvgl
                        font
                        wifi
                        http
                         lvgl_porting
                         player
                        )
# 当使用 PRIVATE 关键字时，表示所链接的库仅用于当前目标的实现，而不会传递给依赖该目标的其他目标。
# 换句话说：
# 当前目标（demo1）会链接这些库并使用它们。
# 其他依赖 demo1的目标（比如另一个库或可执行文件）不会自动链接这些库。