# -*- cmake -*-
#
# Compilation options shared by all Second Life components.

#*****************************************************************************
# We setup four configurations:
# Debug - Full Debug build against Debug libraries
# OptDebug - Debug build against Release libraries
# RelWithDebInfo - Release build with Asserts
# Release - Release build
#*****************************************************************************
include_guard()

include(Variables)
include(Linking)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF) # This slows down build massively

# Optimize static library build dependency targets
set(CMAKE_OPTIMIZE_DEPENDENCIES ON)

# Enable colored compiler diagnostic output
set(CMAKE_COLOR_DIAGNOSTICS ON)

# Position Independent Code/ASLR
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Hidden symbols to reduce binary size
set(CMAKE_C_VISIBILITY_PRESET "hidden")
set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Setup threading options
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)

# Link Time Optimization
if(USE_LTO)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# We want warnings as errors by default
if(NOT VS__DISABLE_FATAL_WARNINGS AND NOT GCC_DISABLE_FATAL_WARNINGS AND NOT CLANG_DISABLE_FATAL_WARNINGS)
  set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
endif()

# Set up our OptDebug target
set(CMAKE_C_FLAGS_OPTDEBUG ${CMAKE_CXX_FLAGS_DEBUG})
set(CMAKE_CXX_FLAGS_OPTDEBUG ${CMAKE_CXX_FLAGS_DEBUG})
set(CMAKE_EXE_LINKER_FLAGS_OPTDEBUG ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
set(CMAKE_MODULE_LINKER_FLAGS_OPTDEBUG ${CMAKE_MODULE_LINKER_FLAGS_DEBUG})
set(CMAKE_SHARED_LINKER_FLAGS_OPTDEBUG ${CMAKE_SHARED_LINKER_FLAGS_DEBUG})
set(CMAKE_STATIC_LINKER_FLAGS_OPTDEBUG ${CMAKE_STATIC_LINKER_FLAGS_DEBUG})

# Need to map libraries to release variants on windows and macos
if (WINDOWS OR DARWIN)
  set(CMAKE_MAP_IMPORTED_CONFIG_OPTDEBUG Release)
endif()

# Debug Global Defines
add_compile_definitions(
  $<$<CONFIG:Debug>:LL_DEBUG=1>
  $<$<CONFIG:Debug>:_DEBUG>
)

# OptDebug Global Defines
add_compile_definitions(
  $<$<CONFIG:OptDebug>:LL_DEBUG=1>
  $<$<CONFIG:OptDebug>:NDEBUG>
)

# RelWithDebInfo Global Defines
add_compile_definitions(
  $<$<CONFIG:RelWithDebInfo>:LL_RELEASE=1>
  $<$<CONFIG:RelWithDebInfo>:LL_RELEASE_WITH_DEBUG_INFO=1>
  $<$<CONFIG:RelWithDebInfo>:NDEBUG=1>
)

# Release Global Defines
add_compile_definitions(
  $<$<CONFIG:Release>:LL_RELEASE=1>
  $<$<CONFIG:Release>:LL_RELEASE_FOR_DOWNLOAD=1>
  $<$<CONFIG:Release>:NDEBUG=1>
)

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

if(RELEASE_CRASH_REPORTING)
  add_compile_definitions(LL_SEND_CRASH_REPORTS=1)
endif()

if(NON_RELEASE_CRASH_REPORTING)
  add_compile_definitions(LL_SEND_CRASH_REPORTS=1)
endif()

# Platform-specific compilation flags.
if(WINDOWS)
  set(CMAKE_MSVC_RUNTIME_CHECKS "$<$<CONFIG:Debug>:StackFrameErrorCheck;UninitializedVariable>")
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT $<IF:$<CONFIG:Debug,OptDebug>,EditAndContinue,ProgramDatabase>)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

  # Don't build DLLs.
  set(BUILD_SHARED_LIBS OFF)

  add_link_options(
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>
    /DEBUG:FULL
    /LARGEADDRESSAWARE
    /NODEFAULTLIB:LIBCMT
    /NODEFAULTLIB:LIBCMTD
    $<$<CONFIG:OptDebug,RelWithDebInfo,Release>:/NODEFAULTLIB:MSVCRTD>
    $<$<CONFIG:Debug>:/NODEFAULTLIB:MSVCRT>
  )

  add_compile_definitions(
    LL_WINDOWS=1
    UNICODE
    _UNICODE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
  )

  # Set windows specific warning supressions
  add_compile_definitions(
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS # Allow use of sprintf etc
    _CRT_NONSTDC_NO_DEPRECATE # Allow use of sprintf etc
    _CRT_OBSOLETE_NO_WARNINGS
    _WINSOCK_DEPRECATED_NO_WARNINGS # Disable deprecated WinSock API warnings
  )

  # Webrtc libraries incompatible with win32 debug builds
  add_compile_definitions(
    $<$<CONFIG:Debug>:DISABLE_WEBRTC=1>
  )

  # Options shared between all configurations
  add_compile_options(
    /EHsc
    /Gy
    /GS
    /GR
    /W3
    /nologo
    $<$<CONFIG:RelWithDebInfo,Release>:/fp:fast>
    /MP
    /permissive-
    /Zc:preprocessor
    /Zc:__cplusplus
    /Zc:inline
    /Zc:wchar_t
  )

  # Debug MSVC Options
  add_compile_options(
    $<$<CONFIG:Debug>:/Od>
    $<$<CONFIG:Debug>:/Ob0>
  )

  # OptDebug MSVC Options
  add_compile_options(
    $<$<CONFIG:OptDebug>:/Od>
    $<$<CONFIG:OptDebug>:/Ob0>
  )

  # RelWithDebInfo MSVC Options
  add_compile_options(
    $<$<CONFIG:Release>:/O2>
  )

  # Release MSVC Options
  add_compile_options(
    $<$<CONFIG:Release>:/O2>
  )

  # We want aggressive inlining on MSVC Release to better match clang/gcc at O3
  string(REPLACE "/Ob1" "/Ob3" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  string(REPLACE "/Ob1" "/Ob3" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  string(REPLACE "/Ob2" "/Ob3" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  string(REPLACE "/Ob2" "/Ob3" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif(WINDOWS)

if(LINUX)
  set(CMAKE_SKIP_RPATH TRUE)

  # LL_IGNORE_SIGCHLD
  # don't catch SIGCHLD in our base application class for the viewer - some of
  # our 3rd party libs may need their *own* SIGCHLD handler to work. Sigh! The
  # viewer doesn't need to catch SIGCHLD anyway.

  add_compile_definitions(
    LL_LINUX=1
    _REENTRANT
    APPID=secondlife
    LL_IGNORE_SIGCHLD
  )

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

  # Options shared between all configs
  add_compile_options(
    $<$<CONFIG:RelWithDebInfo,Release>:-fstack-protector>
    -fexceptions
    -fno-math-errno
    -fno-strict-aliasing
    -fsigned-char
    -g
    -msse2
  )

  # Debug Options
  add_compile_options(
    $<$<CONFIG:Debug>:-O0>
  )

  # OptDebug Options
  add_compile_options(
    $<$<CONFIG:OptDebug>:-Og>
  )

  # RelWithDebInfo Options
  add_compile_options(
    $<$<CONFIG:RelWithDebInfo>:-O3>
  )

  # Release Options
  add_compile_options(
    $<$<CONFIG:Release>:-O3>
  )

  add_link_options(
    "LINKER:-z,relro"
    "LINKER:-z,now"
    "LINKER:--build-id"
    "LINKER:--as-needed"
    "LINKER:--no-undefined"
  )

  # Only turn on headless if we can find osmesa libraries.
  find_package(PkgConfig)
  pkg_check_modules(OSMESA IMPORTED_TARGET GLOBAL osmesa)
  if(OSMESA_FOUND)
    set(BUILD_HEADLESS ON CACHE BOOL "Build headless libraries.")
  endif(OSMESA_FOUND)
endif(LINUX)

if(DARWIN)
  # Use rpath loading on macos
  set(CMAKE_MACOSX_RPATH TRUE)

  # Only generate top-level xcodeproj
  set(CMAKE_XCODE_GENERATE_TOP_LEVEL_PROJECT_ONLY ON)

  # Set up xcode scheme
  set(CMAKE_XCODE_GENERATE_SCHEME ON)
  set(CMAKE_XCODE_SCHEME_LAUNCH_CONFIGURATION "RelWithDebInfo")
  if(ENABLE_ASAN OR ENABLE_UBSAN OR ENABLE_THREADSAN)
    if(ENABLE_ASAN)
      set(CMAKE_XCODE_SCHEME_ADDRESS_SANITIZER ON)
    endif()

    if(ENABLE_UBSAN)
      set(CMAKE_XCODE_UNDEFINED_BEHAVIOUR_SANITIZER ON)
    endif()

    if(ENABLE_THREADSAN)
      set(CMAKE_XCODE_SCHEME_THREAD_SANITIZER ON)
    endif()
  endif()

  # Use dwarf symbols for most libraries and executables for compilation speed
  # per-target overrides applied where needed
  set(CMAKE_XCODE_ATTRIBUTE_GCC_GENERATE_DEBUGGING_SYMBOLS YES)
  set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf")

  set(CMAKE_XCODE_ATTRIBUTE_GCC_STRICT_ALIASING NO)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_FAST_MATH NO)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_X86_VECTOR_INSTRUCTIONS sse4.2)
  # we must hard code this to off for now.  xcode's built in signing does not
  # handle embedded app bundles such as CEF and others. Any signing for local
  # development must be done after the build as we do in viewer_manifest.py for
  # released builds
  # https://stackoverflow.com/a/54296008
  # With Xcode 14.1, apparently you must take drastic steps to prevent
  # implicit signing.
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO)
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
  # "-" represents "Sign to Run Locally" and empty string represents "Do Not Sign"
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "")
  set(CMAKE_XCODE_ATTRIBUTE_DISABLE_MANUAL_TARGET_ORDER_BUILD_WARNING YES)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_WARN_64_TO_32_BIT_CONVERSION NO)

  # Platform define
  add_compile_definitions(LL_DARWIN=1)

  # Ensure debug symbols are always generated
  add_compile_options(-g2 -gdwarf -fno-fast-math -fno-strict-aliasing)

  if(ARCH STREQUAL x86_64)
    add_compile_options(-msse4.2)
  endif()

  # Silence GL deprecation warnings
  add_compile_definitions(GL_SILENCE_DEPRECATION=1)

  # Debug Options
  add_compile_options(
    $<$<CONFIG:Debug>:-O0>
  )

  # OptDebug Options
  add_compile_options(
    $<$<CONFIG:OptDebug>:-Og>
  )

  # RelWithDebInfo Options
  add_compile_options(
    $<$<CONFIG:RelWithDebInfo>:-O3>
  )

  # Release Options
  add_compile_options(
    $<$<CONFIG:Release>:-O3>
  )

  add_link_options($<$<CONFIG:RelWithDebInfo,Release>:LINKER:-dead_strip> LINKER:-dead_strip_dylibs)

  add_link_options("LINKER:-headerpad_max_install_names" "LINKER:-search_paths_first")
endif(DARWIN)

if(LINUX OR DARWIN)
  add_compile_options(-Wall -Wno-sign-compare -Wno-trigraphs -Wno-reorder -Wno-unused-but-set-variable -Wno-unused-variable)

  if(COMPILER_IS_CLANG)
    add_compile_options(-Wno-unused-private-field -Wno-unused-local-typedef -Wno-reorder-ctor)
  endif()

  if(COMPILER_IS_GCC)
    add_compile_options(-Wno-stringop-truncation -Wno-stringop-overflow -Wno-parentheses -Wno-maybe-uninitialized -Wno-unused-local-typedefs)

    # This warning is extremely false positive sensitive, including on libstdc++'s own headers.
    add_compile_options(-Wno-array-bounds)
  endif()

  add_compile_options(-m${ADDRESS_SIZE})
endif()

# Enable support for Drag and Drop
if(OS_DRAG_DROP)
  add_compile_definitions(LL_OS_DRAGDROP_ENABLED=1)
endif(OS_DRAG_DROP)
