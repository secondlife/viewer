# -*- cmake -*-

include(JPEG)
include(PNG)

set(LLIMAGE_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llimage
    ${JPEG_INCLUDE_DIRS}
    )

set(LLIMAGE_LIBRARIES llimage)
