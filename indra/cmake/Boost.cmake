include_guard()
add_library(ll::boost INTERFACE IMPORTED)

if(USE_VCPKG)
    find_package(Boost REQUIRED COMPONENTS context fiber filesystem json program_options thread url)
    target_link_libraries(ll::boost INTERFACE Boost::disable_autolinking Boost::headers Boost::fiber Boost::context Boost::filesystem Boost::thread Boost::program_options Boost::url Boost::json)
    if(WINDOWS)
        find_package(Boost REQUIRED COMPONENTS stacktrace_windbg)
        target_link_libraries(ll::boost INTERFACE Boost::stacktrace_windbg)
    else()
        find_package(Boost REQUIRED COMPONENTS stacktrace_basic)
        target_link_libraries(ll::boost INTERFACE Boost::stacktrace_basic)
    endif()
    return()
endif()

include(Prebuilt)
include(Linking)

use_prebuilt_binary(boost)

# As of sometime between Boost 1.67 and 1.72, Boost libraries are suffixed
# with the address size.
set(addrsfx "-x${ADDRESS_SIZE}")

find_library(BOOST_CONTEXT_LIBRARY
    NAMES
    libboost_context
    libboost_context-mt
    libboost_context-mt${addrsfx}
    boost_context
    boost_context-mt
    boost_context-mt${addrsfx}
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

find_library(BOOST_FIBER_LIBRARY
    NAMES
    libboost_fiber
    libboost_fiber-mt
    libboost_fiber-mt${addrsfx}
    boost_fiber
    boost_fiber-mt
    boost_fiber-mt${addrsfx}
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

find_library(BOOST_FILESYSTEM_LIBRARY
    NAMES
    libboost_filesystem
    libboost_filesystem-mt
    libboost_filesystem-mt${addrsfx}
    boost_filesystem
    boost_filesystem-mt
    boost_filesystem-mt${addrsfx}
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

find_library(BOOST_PROGRAMOPTIONS_LIBRARY
    NAMES
    libboost_program_options
    libboost_program_options-mt
    libboost_program_options-mt${addrsfx}
    boost_program_options
    boost_program_options-mt
    boost_program_options-mt${addrsfx}
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

find_library(BOOST_URL_LIBRARY
    NAMES
    libboost_url
    libboost_url-mt
    libboost_url-mt${addrsfx}
    boost_url
    boost_url-mt
    boost_url-mt${addrsfx}
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::boost INTERFACE
    ${BOOST_FIBER_LIBRARY}
    ${BOOST_CONTEXT_LIBRARY}
    ${BOOST_FILESYSTEM_LIBRARY}
    ${BOOST_PROGRAMOPTIONS_LIBRARY}
    ${BOOST_URL_LIBRARY})

if (LINUX)
    target_link_libraries(ll::boost INTERFACE rt)
endif (LINUX)

target_include_directories(ll::boost SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include/")

target_compile_definitions(ll::boost INTERFACE LL_BOOST=1 BOOST_REGEX_NO_LIB=1)
