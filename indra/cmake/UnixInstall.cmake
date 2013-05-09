if (NOT ${viewer_VERSION})
  MESSAGE(FATAL_ERROR "Viewer version not known!")
endif (NOT ${viewer_VERSION})

set(INSTALL OFF CACHE BOOL
    "Generate install target.")

if (INSTALL)
  set(INSTALL_PREFIX /usr CACHE PATH
      "Top-level installation directory.")

  if (EXISTS /lib64)
    set(_LIB lib64)
  else (EXISTS /lib64)
    set(_LIB lib)
  endif (EXISTS /lib64)

  set(INSTALL_LIBRARY_DIR ${INSTALL_PREFIX}/${_LIB} CACHE PATH
      "Installation directory for read-only shared files.")

  set(INSTALL_SHARE_DIR ${INSTALL_PREFIX}/share CACHE PATH
      "Installation directory for read-only shared files.")

  set(APP_BINARY_DIR ${INSTALL_LIBRARY_DIR}/secondlife-${viewer_VERSION}
      CACHE PATH
      "Installation directory for binaries.")

  set(APP_SHARE_DIR ${INSTALL_SHARE_DIR}/secondlife-${viewer_VERSION}
      CACHE PATH
      "Installation directory for read-only data files.")
endif (INSTALL)
