# -*- cmake -*-
include(Prebuilt)

# There are three possible solutions to provide the llphysicsextensions:
# - The full source package, selected by -DHAVOK:BOOL=ON
# - The stub source package, selected by -DHAVOK:BOOL=OFF
# - The prebuilt package available to those with sublicenses, selected by -DHAVOK_TPV:BOOL=ON

if (INSTALL_PROPRIETARY)
   set(HAVOK ON CACHE BOOL "Use Havok physics library")
endif (INSTALL_PROPRIETARY)

include_guard()
add_library( llphysicsextensions_impl INTERFACE IMPORTED )


# Note that the use_prebuilt_binary macros below do not in fact include binaries;
# the llphysicsextensions_* packages are source only and are built here.
# The source package and the stub package both build libraries of the same name.

if (HAVOK)
   include(Havok)
   use_prebuilt_binary(llphysicsextensions_source)
   set(LLPHYSICSEXTENSIONS_SRC_DIR ${LIBS_PREBUILT_DIR}/llphysicsextensions/src)
   target_link_libraries( llphysicsextensions_impl INTERFACE llphysicsextensions)
   target_compile_definitions( llphysicsextensions_impl INTERFACE LL_HAVOK=1 )
elseif (HAVOK_TPV)
   use_prebuilt_binary(llphysicsextensions_tpv)
   if(WINDOWS)
      target_link_libraries( llphysicsextensions_impl INTERFACE ${ARCH_PREBUILT_DIRS}/llphysicsextensions_tpv.lib)
   else()
      target_link_libraries( llphysicsextensions_impl INTERFACE ${ARCH_PREBUILT_DIRS}/libllphysicsextensions_tpv.a)
   endif()
   target_compile_definitions( llphysicsextensions_impl INTERFACE LL_HAVOK=1 )
else (HAVOK)
   use_prebuilt_binary(llphysicsextensions_stub)
   set(LLPHYSICSEXTENSIONS_SRC_DIR ${LIBS_PREBUILT_DIR}/llphysicsextensions/stub)
   target_link_libraries( llphysicsextensions_impl INTERFACE llphysicsextensionsstub)
endif (HAVOK)

target_include_directories( llphysicsextensions_impl INTERFACE   ${LIBS_PREBUILT_DIR}/include/llphysicsextensions)
