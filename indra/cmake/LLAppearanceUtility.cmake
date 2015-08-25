# -*- cmake -*-
include(Prebuilt)
include(Boost)

# Linux proprietary build only
if (INSTALL_PROPRIETARY)
    if(LINUX)
        use_prebuilt_binary(llappearance_utility)
        set(LLAPPEARANCEUTILITY_SRC_DIR ${LIBS_PREBUILT_DIR}/llappearanceutility/src)
        set(LLAPPEARANCEUTILITY_BIN_DIR ${CMAKE_BINARY_DIR}/llappearanceutility)
    endif (LINUX)
endif (INSTALL_PROPRIETARY)


