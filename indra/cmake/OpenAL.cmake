# -*- cmake -*-
include_guard()

if (USE_OPENAL)
    add_library( ll::openal INTERFACE IMPORTED )
    target_compile_definitions( ll::openal INTERFACE LL_OPENAL=1)

    find_package(OpenAL CONFIG REQUIRED)
    find_package(FreeALUT CONFIG REQUIRED)
    target_link_libraries(ll::openal INTERFACE FreeALUT::alut OpenAL::OpenAL)
endif ()
