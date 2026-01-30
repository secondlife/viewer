include_guard()
add_library(ll::boost INTERFACE IMPORTED)

find_package(Boost REQUIRED COMPONENTS context fiber filesystem json program_options thread url)
target_link_libraries(ll::boost INTERFACE Boost::disable_autolinking Boost::headers Boost::fiber Boost::context Boost::filesystem Boost::program_options Boost::url Boost::json)

if(WINDOWS)
    find_package(Boost REQUIRED COMPONENTS stacktrace_windbg)
    target_link_libraries(ll::boost INTERFACE Boost::stacktrace_windbg)
else()
    find_package(Boost REQUIRED COMPONENTS stacktrace_basic)
    target_link_libraries(ll::boost INTERFACE Boost::stacktrace_basic)
endif()
