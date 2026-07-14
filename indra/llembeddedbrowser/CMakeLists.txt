# -*- cmake -*-

# CEF headers require C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(00-Common)
include(LLCommon)
include(Linking)
include(EmbeddedBrowser)

project(llembeddedbrowser)

set(llembeddedbrowser_SOURCE_FILES
    llembeddedbrowser.cpp
    )

set(llembeddedbrowser_HEADER_FILES
    CMakeLists.txt
    llembeddedbrowser.h
    )

list(APPEND llembeddedbrowser_SOURCE_FILES ${llembeddedbrowser_HEADER_FILES})

add_library(llembeddedbrowser STATIC ${llembeddedbrowser_SOURCE_FILES})

set_target_properties(llembeddedbrowser PROPERTIES PUBLIC_HEADER llembeddedbrowser.h)

target_include_directories(llembeddedbrowser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(llembeddedbrowser llcommon)

ADD_CUSTOM_COMMAND(TARGET llembeddedbrowser POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                   $<TARGET_FILE:llembeddedbrowser>
                   ${SHARED_LIB_STAGING_DIR})
