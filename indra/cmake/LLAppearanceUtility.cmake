# -*- cmake -*-
include(Prebuilt)
include(Boost)

# Linux proprietary build only
if (INSTALL_PROPRIETARY)
    if(LINUX)
        use_prebuilt_binary(llappearance_utility)
        set(LLAPPEARANCEUTILITY_SRC_DIR ${LIBS_PREBUILT_DIR}/llappearanceutility/src)
        set(LLAPPEARANCEUTILITY_BIN_DIR ${CMAKE_BINARY_DIR}/llappearanceutility)
        target_link_libraries(appearance-utility-bin
          ${BOOST_COROUTINE_LIBRARY}
          ${BOOST_CONTEXT_LIBRARY}
          ${BOOST_SYSTEM_LIBRARY})
        target_link_libraries(appearance-utility-headless-bin
          ${BOOST_COROUTINE_LIBRARY}
          ${BOOST_CONTEXT_LIBRARY}
          ${BOOST_SYSTEM_LIBRARY})
    endif (LINUX)
endif (INSTALL_PROPRIETARY)

