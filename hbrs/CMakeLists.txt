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
libmp4v2.a
libvisca.a
libSDL_ttf.a
libSDL.a
libfreetype.a
${CMAKE_SOURCE_DIR}/3rdparty/lib/srs_librtmp.a
librtmp.a
libssl.a
libcrypto.a
libz.a
pthread
dl
rt
m
)
