# -*- cmake -*-
include(Prebuilt)

include_guard()

add_library( ll::boost INTERFACE IMPORTED )
if( USE_CONAN )
  target_link_libraries( ll::boost INTERFACE CONAN_PKG::boost )
  target_compile_definitions( ll::boost INTERFACE BOOST_ALLOW_DEPRECATED_HEADERS BOOST_BIND_GLOBAL_PLACEHOLDERS )
  return()
endif()

use_prebuilt_binary(boost)

# As of sometime between Boost 1.67 and 1.72, Boost libraries are suffixed
# with the address size.
set(addrsfx "-x${ADDRESS_SIZE}")

if (WINDOWS)
  target_link_libraries( ll::boost INTERFACE
          libboost_context-mt${addrsfx}
          libboost_fiber-mt${addrsfx}
          libboost_filesystem-mt${addrsfx}
          libboost_program_options-mt${addrsfx}
          libboost_regex-mt${addrsfx}
          libboost_system-mt${addrsfx}
          libboost_thread-mt${addrsfx}
          libboost_url-mt${addrsfx}
          )
elseif (DARWIN)
  target_link_libraries( ll::boost INTERFACE
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_context-mt.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_fiber-mt.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_filesystem-mt.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_program_options-mt.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_regex-mt.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_system-mt.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_thread-mt.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_url-mt.a
          )
else ()
  target_link_libraries( ll::boost INTERFACE
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_context-mt${addrsfx}.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_fiber-mt${addrsfx}.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_filesystem-mt${addrsfx}.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_program_options-mt${addrsfx}.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_regex-mt${addrsfx}.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_system-mt${addrsfx}.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_thread-mt${addrsfx}.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libboost_url-mt${addrsfx}.a
          )
endif ()

