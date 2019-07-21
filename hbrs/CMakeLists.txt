file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
add_executable(3531 ${SOURCES})


target_link_libraries(3531 
libeasylogger.a
libaacenc.a
jsoncpp
libmpi.a
libhdmi.a
libVoiceEngine.a
libaec.a
libanr.a
libresampler.a
libtde.a
libjpeg.a
libpciv.a
libevent.a
pthread
dl
rt
m
)
