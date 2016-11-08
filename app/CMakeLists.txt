
cmake_minimum_required(VERSION 3.4.1)

include_directories(include/)

add_library( native-audio SHARED
             src/main/cpp/native-audio.cpp)


target_link_libraries(native-audio
                      android
                      log
                      OpenSLES)