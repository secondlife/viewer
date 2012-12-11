# -*- cmake -*-

project(llaudio)

include(00-Common)
include(Audio)
include(LLAudio)
include(FMOD)
include(OPENAL)
include(LLCommon)
include(LLMath)
include(LLMessage)
include(LLVFS)

include_directories(
    ${LLAUDIO_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${OGG_INCLUDE_DIRS}
    ${VORBISENC_INCLUDE_DIRS}
    ${VORBISFILE_INCLUDE_DIRS}
    ${VORBIS_INCLUDE_DIRS}
    ${OPENAL_LIB_INCLUDE_DIRS}
    ${FREEAULT_LIB_INCLUDE_DIRS}
    ${FMOD_INCLUDE_DIR}
    )

set(llaudio_SOURCE_FILES
    llaudioengine.cpp
    lllistener.cpp
    llaudiodecodemgr.cpp
    llvorbisencode.cpp
    )

set(llaudio_HEADER_FILES
    CMakeLists.txt

    llaudioengine.h
    lllistener.h
    llaudiodecodemgr.h
    llvorbisencode.h
    llwindgen.h
    )

if (FMOD)
    include_directories(
        ${FMOD_INCLUDE_DIR}
        )

    list(APPEND llaudio_SOURCE_FILES
         llaudioengine_fmod.cpp
         lllistener_fmod.cpp
         llstreamingaudio_fmod.cpp
         )

    list(APPEND llaudio_HEADER_FILES
         llaudioengine_fmod.h
         lllistener_fmod.h
         llstreamingaudio_fmod.h
         )

    if (LINUX OR DARWIN)
      set_source_files_properties(llaudioengine_fmod.cpp
                                  llstreamingaudio_fmod.cpp
                                  COMPILE_FLAGS -Wno-write-strings)
    endif (LINUX OR DARWIN)
endif (FMOD)

if (OPENAL)
  list(APPEND llaudio_SOURCE_FILES
    llaudioengine_openal.cpp
    lllistener_openal.cpp
    )
  
  list(APPEND llaudio_HEADER_FILES
    llaudioengine_openal.h
    lllistener_openal.h
    )
endif (OPENAL)

set_source_files_properties(${llaudio_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llaudio_SOURCE_FILES ${llaudio_HEADER_FILES})

add_library (llaudio ${llaudio_SOURCE_FILES})
target_link_libraries(
    llaudio
    ${LLCOMMON_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${VORBISENC_LIBRARIES}
    ${VORBISFILE_LIBRARIES}
    ${VORBIS_LIBRARIES}
    ${OGG_LIBRARIES}
    )
