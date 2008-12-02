# -*- cmake -*-

project(llaudio)

include(00-Common)
include(Audio)
include(FMOD)
include(OPENAL)
include(LLCommon)
include(LLMath)
include(LLMessage)
include(LLVFS)
include(LLMedia)

include_directories(
    ${FMOD_INCLUDE_DIR}
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
    ${LLMEDIA_INCLUDE_DIRS}
    )

set(llaudio_SOURCE_FILES
    audioengine.cpp
    listener.cpp
    llaudiodecodemgr.cpp
    vorbisdecode.cpp
    vorbisencode.cpp
    )

set(llaudio_HEADER_FILES
    CMakeLists.txt

    audioengine.h
    listener.h
    llaudiodecodemgr.h
    vorbisdecode.h
    vorbisencode.h
    windgen.h
    )

if (FMOD)
    list(APPEND llaudio_SOURCE_FILES
         audioengine_fmod.cpp
         listener_fmod.cpp
         )

    list(APPEND llaudio_HEADER_FILES
         audioengine_fmod.h
         listener_fmod.h
         )

    if (LINUX)
      if (${CXX_VERSION} MATCHES "4.[23]")
        set_source_files_properties(audioengine_fmod.cpp
                                    COMPILE_FLAGS -Wno-error=write-strings)
      endif (${CXX_VERSION} MATCHES "4.[23]")
    endif (LINUX)
endif (FMOD)

if (OPENAL)
  list(APPEND llaudio_SOURCE_FILES
    audioengine_openal.cpp
    listener_openal.cpp
    )
  
  list(APPEND llaudio_HEADER_FILES
    audioengine_openal.h
    listener_openal.h
    )
endif (OPENAL)

set_source_files_properties(${llaudio_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llaudio_SOURCE_FILES ${llaudio_HEADER_FILES})

add_library (llaudio ${llaudio_SOURCE_FILES})
target_link_libraries(
    llaudio
    ${VORBISENC_LIBRARIES}
    ${VORBISFILE_LIBRARIES}
    ${VORBIS_LIBRARIES}
    ${OGG_LIBRARIES}
    )
