# -*- cmake -*-

include(Audio)

set(LLAUDIO_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llaudio
    )

set(LLAUDIO_LIBRARIES
    llaudio
    ${VORBISENC_LIBRARIES}
    ${VORBISFILE_LIBRARIES}
    ${VORBIS_LIBRARIES}
    ${OGG_LIBRARIES}
    )
