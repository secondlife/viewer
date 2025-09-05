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

    find_library(BOOST_CONTEXT_LIBRARY
        NAMES
        libboost_context-mt
        libboost_context-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_FIBER_LIBRARY
        NAMES
        libboost_fiber-mt
        libboost_fiber-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_FILESYSTEM_LIBRARY
        NAMES
        libboost_filesystem-mt
        libboost_filesystem-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_PROGRAMOPTIONS_LIBRARY
        NAMES
        libboost_program_options-mt
        libboost_program_options-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_REGEX_LIBRARY
        NAMES
        libboost_regex-mt
        libboost_regex-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_SYSTEM_LIBRARY
        NAMES
        libboost_system-mt
        libboost_system-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_THREAD_LIBRARY
        NAMES
        libboost_thread-mt
        libboost_thread-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_URL_LIBRARY
        NAMES
        libboost_url-mt
        libboost_url-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

else (WINDOWS)

    find_library(BOOST_CONTEXT_LIBRARY
       NAMES
       boost_context-mt
       boost_context-mt${addrsfx}
       PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_FIBER_LIBRARY
        NAMES
        boost_fiber-mt
        boost_fiber-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_FILESYSTEM_LIBRARY
        NAMES
        boost_filesystem-mt
        boost_filesystem-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_PROGRAMOPTIONS_LIBRARY
        NAMES
        boost_program_options-mt
        boost_program_options-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_REGEX_LIBRARY
        NAMES
        boost_regex-mt
        boost_regex-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_SYSTEM_LIBRARY
        NAMES
        boost_system-mt
        boost_system-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_THREAD_LIBRARY
        NAMES
        boost_thread-mt
        boost_thread-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(BOOST_URL_LIBRARY
        NAMES
        boost_url-mt
        boost_url-mt${addrsfx}
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

endif (WINDOWS)

target_link_libraries(ll::boost INTERFACE
    ${BOOST_FIBER_LIBRARY}
    ${BOOST_CONTEXT_LIBRARY}
    ${BOOST_FILESYSTEM_LIBRARY}
    ${BOOST_PROGRAMOPTIONS_LIBRARY}
    ${BOOST_REGEX_LIBRARY}
    ${BOOST_SYSTEM_LIBRARY}
    ${BOOST_THREAD_LIBRARY}
    ${BOOST_URL_LIBRARY})

if (LINUX)
    target_link_libraries(ll::boost INTERFACE rt)
endif (LINUX)

