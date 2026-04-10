cmake_minimum_required(VERSION 3.15)

project(app1 VERSION 1.0)

add_compile_options(-Wno-psabi)

# 第三方库安装根目录
set(THIRD_PARTY_ROOT /home/xiaoyubb/xiangmu/app_sdk_v2/third_party/install)

# 添加头文件搜索路径
include_directories(
    ${THIRD_PARTY_ROOT}/alsa/include
    ${THIRD_PARTY_ROOT}/opus/include
    ${THIRD_PARTY_ROOT}/boost/include
)

# 添加库文件搜索路径
link_directories(
    ${THIRD_PARTY_ROOT}/alsa/lib
    ${THIRD_PARTY_ROOT}/opus/lib
    ${THIRD_PARTY_ROOT}/boost/lib
)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(SIMULATOR_LINUX)
    message("building linux x86")
    add_subdirectory(platform/x86linux) 
else()
    message("building t113")
    add_subdirectory(platform/t113)
    add_subdirectory(app_sound)
    add_subdirectory(app_control_center)
    add_subdirectory(app_xiaozhi)
endif()
add_subdirectory(component/osal)
add_subdirectory(component/wifi)
add_subdirectory(component/net)
add_subdirectory(component/player)
add_subdirectory(component/font)
add_subdirectory(component/usb_hid)
add_subdirectory(component/bt_audio)
add_subdirectory(lvgl)
#add_subdirectory(app)
add_subdirectory(app1)