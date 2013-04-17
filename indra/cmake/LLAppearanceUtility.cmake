# -*- cmake -*-
include(Prebuilt)

# Linux proprietary build only
if (INSTALL_PROPRIETARY)
    if(LINUX)
        use_prebuilt_binary(llappearanceutility-source)
        set(LLAPPEARANCEUTILITY_SRC_DIR ${LIBS_PREBUILT_DIR}/llappearanceutility/src)
        set(LLAPPEARANCEUTILITY_BIN_DIR ${CMAKE_BINARY_DIR}/llappearanceutility)
    endif (LINUX)
endif (INSTALL_PROPRIETARY)

