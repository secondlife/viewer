include_guard()
add_library(ll::bugsplat INTERFACE IMPORTED)

if(USE_BUGSPLAT AND NOT LINUX)
    if(WINDOWS)
        find_library(BUGSPLAT_LIBRARIES BugSplat64 REQUIRED)
        target_link_libraries( ll::bugsplat INTERFACE ${BUGSPLAT_LIBRARIES})

        find_path(BUGSPLAT_INCLUDE_DIRS "bugsplat/BugSplat.h" REQUIRED)
        target_include_directories(ll::bugsplat SYSTEM INTERFACE ${BUGSPLAT_INCLUDE_DIRS})
    elseif(DARWIN)
        find_library(BUGSPLAT_LIBRARIES BugsplatMac REQUIRED)
        find_library(CRASHREPORTED_LIBRARIES CrashReporter REQUIRED)
        find_library(HOCKEYSDK_LIBRARIES HockeySDK REQUIRED)
        target_link_libraries( ll::bugsplat INTERFACE
                ${BUGSPLAT_LIBRARIES}
                ${CRASHREPORTED_LIBRARIES}
                ${HOCKEYSDK_LIBRARIES}
                )
    else ()
        message(FATAL_ERROR "BugSplat is not supported; add -DUSE_BUGSPLAT=OFF")
    endif()

    if( NOT BUGSPLAT_DB )
        message(FATAL_ERROR "You need to set BUGSPLAT_DB when setting USE_BUGSPLAT")
    endif()

    target_compile_definitions(ll::bugsplat INTERFACE LL_BUGSPLAT=1)
endif()

