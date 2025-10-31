# -*- cmake -*-
#
# Compilation options shared by all Second Life components.

#*****************************************************************************
#   It's important to realize that CMake implicitly concatenates
#   CMAKE_CXX_FLAGS with (e.g.) CMAKE_CXX_FLAGS_RELEASE for Release builds. So
#   set switches in CMAKE_CXX_FLAGS that should affect all builds, but in
#   CMAKE_CXX_FLAGS_RELEASE or CMAKE_CXX_FLAGS_RELWITHDEBINFO for switches
#   that should affect only that build variant.
#
#   Also realize that CMAKE_CXX_FLAGS may already be partially populated on
#   entry to this file.
#
#   Additionally CMAKE_C_FLAGS is prepended to CMAKE_CXX_FLAGS_RELEASE and
#   CMAKE_CXX_FLAGS_RELWITHDEBINFO which risks having flags overriden by cmake
#   inserting additional options that are part of the build config type.
#*****************************************************************************
include_guard()

include(Variables)
include(Linking)

# We go to some trouble to set LL_BUILD to the set of relevant compiler flags.
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} $ENV{LL_BUILD_RELEASE}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} $ENV{LL_BUILD_RELWITHDEBINFO}")

# Given that, all the flags you see added below are flags NOT present in
# https://bitbucket.org/lindenlab/viewer-build-variables/src/tip/variables.
# Before adding new ones here, it's important to ask: can this flag really be
# applied to the viewer only, or should/must it be applied to all 3p libraries
# as well?

# Portable compilation flags.
add_compile_definitions(ADDRESS_SIZE=${ADDRESS_SIZE})
# Because older versions of Boost.Bind dumped placeholders _1, _2 et al. into
# the global namespace, Boost now requires either BOOST_BIND_NO_PLACEHOLDERS
# to avoid that or BOOST_BIND_GLOBAL_PLACEHOLDERS to state that we require it
# -- which we do. Without one or the other, we get a ton of Boost warnings.
add_compile_definitions(BOOST_BIND_GLOBAL_PLACEHOLDERS)

# Force enable SSE2 instructions in GLM per the manual
# https://github.com/g-truc/glm/blob/master/manual.md#section2_10
add_compile_definitions(GLM_FORCE_DEFAULT_ALIGNED_GENTYPES=1 GLM_ENABLE_EXPERIMENTAL=1)

# SSE2NEON throws a pointless warning when compiler optimizations are enabled
add_compile_definitions(SSE2NEON_SUPPRESS_WARNINGS=1)

# Configure crash reporting
set(RELEASE_CRASH_REPORTING OFF CACHE BOOL "Enable use of crash reporting in release builds")
set(NON_RELEASE_CRASH_REPORTING OFF CACHE BOOL "Enable use of crash reporting in developer builds")

if(RELEASE_CRASH_REPORTING)
  add_compile_definitions( LL_SEND_CRASH_REPORTS=1)
endif()

if(NON_RELEASE_CRASH_REPORTING)
  add_compile_definitions( LL_SEND_CRASH_REPORTS=1)
endif()

set(USE_LTO OFF CACHE BOOL "Enable Link Time Optimization")
if(USE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# Don't bother with a MinSizeRel or Debug builds.
set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Release" CACHE STRING "Supported build types." FORCE)

# Platform-specific compilation flags.

if (WINDOWS)
  # Don't build DLLs.
  set(BUILD_SHARED_LIBS OFF)

  # for "backwards compatibility", cmake sneaks in the Zm1000 option which royally
  # screws incredibuild. this hack disables it.
  # for details see: http://connect.microsoft.com/VisualStudio/feedback/details/368107/clxx-fatal-error-c1027-inconsistent-values-for-ym-between-creation-and-use-of-precompiled-headers
  # http://www.ogre3d.org/forums/viewtopic.php?f=2&t=60015
  # http://www.cmake.org/pipermail/cmake/2009-September/032143.html
  string(REPLACE "/Zm1000" " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

  add_link_options(/LARGEADDRESSAWARE
          /NODEFAULTLIB:LIBCMT
          /IGNORE:4099)

  add_compile_definitions(
      WIN32_LEAN_AND_MEAN
      NOMINMAX
#     DOM_DYNAMIC                     # For shared library colladadom
      _CRT_SECURE_NO_WARNINGS         # Allow use of sprintf etc
      _CRT_NONSTDC_NO_DEPRECATE       # Allow use of sprintf etc
      _CRT_OBSOLETE_NO_WARNINGS
      _WINSOCK_DEPRECATED_NO_WARNINGS # Disable deprecated WinSock API warnings
      )
  add_compile_options(
          /Zo
          /GS
          /TP
          /W3
          /c
          /Zc:forScope
          /nologo
          /Oy-
          /fp:fast
          /MP
          /permissive-
      )

  # Nicky: x64 implies SSE2
  if( ADDRESS_SIZE EQUAL 32 )
    add_compile_options( /arch:SSE2 )
  endif()

  # Are we using the crummy Visual Studio KDU build workaround?
  if (NOT VS_DISABLE_FATAL_WARNINGS)
    add_compile_options(/WX)
  endif (NOT VS_DISABLE_FATAL_WARNINGS)

  #ND: When using something like buildcache (https://github.com/mbitsnbites/buildcache)
  # to make those wrappers work /Zi must be changed to /Z7, as /Zi due to it's nature is not compatible with caching
  if (${CMAKE_CXX_COMPILER_LAUNCHER} MATCHES ".*cache.*")
    add_compile_options( /Z7 )
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  endif()
endif (WINDOWS)


if (LINUX)
  set(CMAKE_SKIP_RPATH TRUE)

   # LL_IGNORE_SIGCHLD
   # don't catch SIGCHLD in our base application class for the viewer - some of
   # our 3rd party libs may need their *own* SIGCHLD handler to work. Sigh! The
   # viewer doesn't need to catch SIGCHLD anyway.

  add_compile_definitions(
          _REENTRANT
          APPID=secondlife
          LL_IGNORE_SIGCHLD
  )

  option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
  option(ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)
  option(ENABLE_THREADSAN "Enable Thread Sanitizer" OFF)
  if(ENABLE_ASAN OR ENABLE_UBSAN OR ENABLE_THREADSAN)
    set(GCC_DISABLE_FATAL_WARNINGS ON) # Disable warnings as errors during sanitizer builds due to false positives

    add_compile_options(
      -U_FORTIFY_SOURCE
      -fno-omit-frame-pointer
      -fno-common
      -fsanitize-recover=all
    )

    # libwebrtc is incompatible with sanitizers
    set(DISABLE_WEBRTC ON)
    add_compile_definitions(DISABLE_WEBRTC=1)

    if(ENABLE_ASAN)
      add_compile_options(-fsanitize=address)
      add_link_options(-fsanitize=address)
    endif()

    if(ENABLE_UBSAN)
      add_compile_options(-fsanitize=undefined)
      add_link_options(-fsanitize=undefined)
    endif()

    if(ENABLE_THREADSAN)
      add_compile_options(-fsanitize=thread)
      add_link_options(-fsanitize=thread)
    endif()
  else()
    add_compile_definitions($<$<CONFIG:Release>:_FORTIFY_SOURCE=2>)
  endif()

  add_compile_options(
          $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-fstack-protector>
          -fexceptions
          -fno-math-errno
          -fno-strict-aliasing
          -fsigned-char
          -g
          -msse2
          -pthread
          -fvisibility=hidden
  )

  add_link_options(
          "LINKER:-z,relro"
          "LINKER:-z,now"
          "LINKER:--build-id"
          "LINKER:--as-needed"
          "LINKER:--no-undefined"
  )
endif (LINUX)

if (DARWIN)
  # Use rpath loading on macos
  set(CMAKE_MACOSX_RPATH TRUE)

  set(CMAKE_CXX_LINK_FLAGS "-Wl,-headerpad_max_install_names,-search_paths_first")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_CXX_LINK_FLAGS}")

  # Ensure debug symbols are always generated
  add_compile_options(-g --debug) # --debug is a clang synonym for -g that bypasses cmake behaviors

  # Silence GL deprecation warnings
  add_compile_definitions(GL_SILENCE_DEPRECATION=1)
endif(DARWIN)

if (LINUX OR DARWIN)
  add_compile_options(-Wall -Wno-sign-compare -Wno-trigraphs -Wno-reorder -Wno-unused-but-set-variable -Wno-unused-variable)

  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_compile_options(-Wno-unused-local-typedef)
  endif()

  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-stringop-truncation -Wno-stringop-overflow -Wno-parentheses -Wno-maybe-uninitialized -Wno-unused-local-typedefs)
  endif ()

  if (NOT GCC_DISABLE_FATAL_WARNINGS AND NOT CLANG_DISABLE_FATAL_WARNINGS)
    add_compile_options(-Werror)
  endif ()

  add_compile_options(-m${ADDRESS_SIZE})
endif (LINUX OR DARWIN)

