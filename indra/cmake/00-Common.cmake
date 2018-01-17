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

if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

include(Variables)

# We go to some trouble to set LL_BUILD to the set of relevant compiler flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{LL_BUILD}")
# Given that, all the flags you see added below are flags NOT present in
# https://bitbucket.org/lindenlab/viewer-build-variables/src/tip/variables.
# Before adding new ones here, it's important to ask: can this flag really be
# applied to the viewer only, or should/must it be applied to all 3p libraries
# as well?

# Portable compilation flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DADDRESS_SIZE=${ADDRESS_SIZE}")

# Configure crash reporting
set(RELEASE_CRASH_REPORTING OFF CACHE BOOL "Enable use of crash reporting in release builds")
set(NON_RELEASE_CRASH_REPORTING OFF CACHE BOOL "Enable use of crash reporting in developer builds")

if(RELEASE_CRASH_REPORTING)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DLL_SEND_CRASH_REPORTS=1")
endif()

if(NON_RELEASE_CRASH_REPORTING)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DLL_SEND_CRASH_REPORTS=1")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLL_SEND_CRASH_REPORTS=1")
endif()  

# Don't bother with a MinSizeRel build.
set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Release;Debug" CACHE STRING
    "Supported build types." FORCE)


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

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO 
      "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Zo"
      CACHE STRING "C++ compiler release-with-debug options" FORCE)
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} ${LL_CXX_FLAGS} /Zo"
      CACHE STRING "C++ compiler release options" FORCE)
  # zlib has assembly-language object files incompatible with SAFESEH
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE /SAFESEH:NO /NODEFAULTLIB:LIBCMT /IGNORE:4099")

  set(CMAKE_CXX_STANDARD_LIBRARIES "")
  set(CMAKE_C_STANDARD_LIBRARIES "")

  add_definitions(
      /DNOMINMAX
#      /DDOM_DYNAMIC            # For shared library colladadom
      )
  add_compile_options(
      /GS
      /TP
      /W3
      /c
      /Zc:forScope
      /nologo
      /Oy-
#      /arch:SSE2
      /fp:fast
      )

  # Nicky: x64 implies SSE2
  if( ADDRESS_SIZE EQUAL 32 )
    add_definitions( /arch:SSE2 )
  endif()
     
  # Are we using the crummy Visual Studio KDU build workaround?
  if (NOT VS_DISABLE_FATAL_WARNINGS)
    add_definitions(/WX)
  endif (NOT VS_DISABLE_FATAL_WARNINGS)
endif (WINDOWS)


if (LINUX)
  set(CMAKE_SKIP_RPATH TRUE)

  add_definitions(-D_FORTIFY_SOURCE=2)

  set(CMAKE_CXX_FLAGS "-Wno-deprecated -Wno-unused-but-set-variable -Wno-unused-variable ${CMAKE_CXX_FLAGS}")

  # gcc 4.3 and above don't like the LL boost and also
  # cause warnings due to our use of deprecated headers
  add_definitions(-Wno-parentheses)

  add_definitions(
      -D_REENTRANT
      )
  add_compile_options(
      -fexceptions
      -fno-math-errno
      -fno-strict-aliasing
      -fsigned-char
      -msse2
      -mfpmath=sse
      -pthread
      )

  # force this platform to accept TOS via external browser
  add_definitions(-DEXTERNAL_TOS)

  add_definitions(-DAPPID=secondlife)
  add_compile_options(-fvisibility=hidden)
  # don't catch SIGCHLD in our base application class for the viewer - some of
  # our 3rd party libs may need their *own* SIGCHLD handler to work. Sigh! The
  # viewer doesn't need to catch SIGCHLD anyway.
  add_definitions(-DLL_IGNORE_SIGCHLD)
  if (ADDRESS_SIZE EQUAL 32)
    add_compile_options(-march=pentium4)
  endif (ADDRESS_SIZE EQUAL 32)
  #add_compile_options(-ftree-vectorize) # THIS CRASHES GCC 3.1-3.2
  if (NOT USESYSTEMLIBS)
    # this stops us requiring a really recent glibc at runtime
    add_compile_options(-fno-stack-protector)
    # linking can be very memory-hungry, especially the final viewer link
    set(CMAKE_CXX_LINK_FLAGS "-Wl,--no-keep-memory")
  endif (NOT USESYSTEMLIBS)

  set(CMAKE_CXX_FLAGS_DEBUG "-fno-inline ${CMAKE_CXX_FLAGS_DEBUG}")
endif (LINUX)


if (DARWIN)
  set(CMAKE_CXX_LINK_FLAGS "-Wl,-headerpad_max_install_names,-search_paths_first")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
  set(DARWIN_extra_cstar_flags "-Wno-unused-local-typedef -Wno-deprecated-declarations")
  # Ensure that CMAKE_CXX_FLAGS has the correct -g debug information format --
  # see Variables.cmake.
  string(REPLACE "-gdwarf-2" "-g${CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT}"
    CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  # The viewer code base can now be successfully compiled with -std=c++14. But
  # turning that on in the generic viewer-build-variables/variables file would
  # potentially require tweaking each of our ~50 third-party library builds.
  # Until we decide to set -std=c++14 in viewer-build-variables/variables, set
  # it locally here: we want to at least prevent inadvertently reintroducing
  # viewer code that would fail with C++14.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DARWIN_extra_cstar_flags} -std=c++14")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  ${DARWIN_extra_cstar_flags}")
  # NOTE: it's critical that the optimization flag is put in front.
  # NOTE: it's critical to have both CXX_FLAGS and C_FLAGS covered.
## Really?? On developer machines too?
##set(ENABLE_SIGNING TRUE)
##set(SIGNING_IDENTITY "Developer ID Application: Linden Research, Inc.")
endif (DARWIN)


if (LINUX OR DARWIN)
  if (CMAKE_CXX_COMPILER MATCHES ".*clang")
    set(CMAKE_COMPILER_IS_CLANGXX 1)
  endif (CMAKE_CXX_COMPILER MATCHES ".*clang")

  if (CMAKE_COMPILER_IS_GNUCXX)
    set(GCC_WARNINGS "-Wall -Wno-sign-compare -Wno-trigraphs")
  elseif (CMAKE_COMPILER_IS_CLANGXX)
    set(GCC_WARNINGS "-Wall -Wno-sign-compare -Wno-trigraphs")
  endif()

  if (NOT GCC_DISABLE_FATAL_WARNINGS)
    set(GCC_WARNINGS "${GCC_WARNINGS} -Werror")
  endif (NOT GCC_DISABLE_FATAL_WARNINGS)

  set(GCC_CXX_WARNINGS "${GCC_WARNINGS} -Wno-reorder -Wno-non-virtual-dtor")

  set(CMAKE_C_FLAGS "${GCC_WARNINGS} ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${GCC_CXX_WARNINGS} ${CMAKE_CXX_FLAGS}")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m${ADDRESS_SIZE}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m${ADDRESS_SIZE}")
endif (LINUX OR DARWIN)


if (USESYSTEMLIBS)
  add_definitions(-DLL_USESYSTEMLIBS=1)

  if (LINUX AND ADDRESS_SIZE EQUAL 32)
    add_definitions(-march=pentiumpro)
  endif (LINUX AND ADDRESS_SIZE EQUAL 32)

else (USESYSTEMLIBS)
  set(${ARCH}_linux_INCLUDES
      ELFIO
      atk-1.0
      glib-2.0
      gstreamer-0.10
      gtk-2.0
      pango-1.0
      )
endif (USESYSTEMLIBS)

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
