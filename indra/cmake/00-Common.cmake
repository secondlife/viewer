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
#*****************************************************************************
include_guard()

include(Variables)

# We go to some trouble to set LL_BUILD to the set of relevant compiler flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{LL_BUILD}")
# Given that, all the flags you see added below are flags NOT present in
# https://bitbucket.org/lindenlab/viewer-build-variables/src/tip/variables.
# Before adding new ones here, it's important to ask: can this flag really be
# applied to the viewer only, or should/must it be applied to all 3p libraries
# as well?

# Portable compilation flags.
add_compile_definitions( ADDRESS_SIZE=${ADDRESS_SIZE})
# Because older versions of Boost.Bind dumped placeholders _1, _2 et al. into
# the global namespace, Boost now requires either BOOST_BIND_NO_PLACEHOLDERS
# to avoid that or BOOST_BIND_GLOBAL_PLACEHOLDERS to state that we require it
# -- which we do. Without one or the other, we get a ton of Boost warnings.
add_compile_definitions(BOOST_BIND_GLOBAL_PLACEHOLDERS)

# Force enable SSE2 instructions in GLM per the manual
# https://github.com/g-truc/glm/blob/master/manual.md#section2_10
add_compile_definitions(GLM_FORCE_DEFAULT_ALIGNED_GENTYPES=1 GLM_FORCE_SSE2=1)

# Configure crash reporting
set(RELEASE_CRASH_REPORTING OFF CACHE BOOL "Enable use of crash reporting in release builds")
set(NON_RELEASE_CRASH_REPORTING OFF CACHE BOOL "Enable use of crash reporting in developer builds")

if(RELEASE_CRASH_REPORTING)
  add_compile_definitions( LL_SEND_CRASH_REPORTS=1)
endif()

if(NON_RELEASE_CRASH_REPORTING)
  add_compile_definitions( LL_SEND_CRASH_REPORTS=1)
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

  # zlib has assembly-language object files incompatible with SAFESEH
  add_link_options(/LARGEADDRESSAWARE
          /SAFESEH:NO
          /NODEFAULTLIB:LIBCMT
          /IGNORE:4099)

  add_definitions(
      -DNOMINMAX
#      /DDOM_DYNAMIC            # For shared library colladadom
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
  if( ${CMAKE_CXX_COMPILER_LAUNCHER} MATCHES ".*cache.*")
    add_compile_options( /Z7 )
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  endif()

  # Allow use of sprintf etc
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif (WINDOWS)


if (LINUX)
  set(CMAKE_SKIP_RPATH TRUE)

   # EXTERNAL_TOS
   # force this platform to accept TOS via external browser

   # LL_IGNORE_SIGCHLD
   # don't catch SIGCHLD in our base application class for the viewer - some of
   # our 3rd party libs may need their *own* SIGCHLD handler to work. Sigh! The
   # viewer doesn't need to catch SIGCHLD anyway.

  add_compile_definitions(
          _REENTRANT
          _FORTIFY_SOURCE=2
          EXTERNAL_TOS
          APPID=secondlife
          LL_IGNORE_SIGCHLD
  )
  add_compile_options(
          -fexceptions
          -fno-math-errno
          -fno-strict-aliasing
          -fsigned-char
          -msse2
          -mfpmath=sse
          -pthread
          -Wno-parentheses
          -Wno-deprecated
          -fvisibility=hidden
  )

  if (ADDRESS_SIZE EQUAL 32)
    add_compile_options(-march=pentium4)
  endif (ADDRESS_SIZE EQUAL 32)

  # this stops us requiring a really recent glibc at runtime
  add_compile_options(-fno-stack-protector)
  # linking can be very memory-hungry, especially the final viewer link
  set(CMAKE_CXX_LINK_FLAGS "-Wl,--no-keep-memory")

  set(CMAKE_CXX_FLAGS_DEBUG "-fno-inline ${CMAKE_CXX_FLAGS_DEBUG}")
endif (LINUX)


if (DARWIN)
  # Warnings should be fatal -- thanks, Nicky Perian, for spotting reversed default
  set(CLANG_DISABLE_FATAL_WARNINGS OFF)
  set(CMAKE_CXX_LINK_FLAGS "-Wl,-headerpad_max_install_names,-search_paths_first")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
  set(DARWIN_extra_cstar_flags "-Wno-unused-local-typedef -Wno-deprecated-declarations")
  # Ensure that CMAKE_CXX_FLAGS has the correct -g debug information format --
  # see Variables.cmake.
  string(REPLACE "-gdwarf-2" "-g${CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT}"
    CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DARWIN_extra_cstar_flags}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  ${DARWIN_extra_cstar_flags}")
  # NOTE: it's critical that the optimization flag is put in front.
  # NOTE: it's critical to have both CXX_FLAGS and C_FLAGS covered.
## Really?? On developer machines too?
##set(ENABLE_SIGNING TRUE)
##set(SIGNING_IDENTITY "Developer ID Application: Linden Research, Inc.")

  # required for clang-15/xcode-15 since our boost package still uses deprecated std::unary_function/binary_function
  # see https://developer.apple.com/documentation/xcode-release-notes/xcode-15-release-notes#C++-Standard-Library
  add_compile_definitions(_LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION)
endif (DARWIN)

if (LINUX OR DARWIN)
  set(GCC_WARNINGS -Wall -Wno-sign-compare -Wno-trigraphs)

  if (NOT GCC_DISABLE_FATAL_WARNINGS)
    list(APPEND GCC_WARNINGS -Werror)
  endif (NOT GCC_DISABLE_FATAL_WARNINGS)

  list(APPEND GCC_WARNINGS -Wno-reorder -Wno-non-virtual-dtor )

  if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
    list(APPEND GCC_WARNINGS -Wno-unused-but-set-variable -Wno-unused-variable )
  endif()

  add_compile_options(${GCC_WARNINGS})
  add_compile_options(-m${ADDRESS_SIZE})
endif (LINUX OR DARWIN)



