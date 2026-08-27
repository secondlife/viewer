# -*- cmake -*-

project(llrender)

include(00-Common)
include(OpenGL)
include(FreeType)
include(LLCommon)
include(LLImage)
include(LLWindow)

set(llrender_SOURCE_FILES
    llcubemap.cpp
    llcubemaparray.cpp
    llfontbitmapcache.cpp
    llfontfreetype.cpp
    llfontfreetypesvg.cpp
    llfontgl.cpp
    llfontvertexbuffer.cpp
    llfontregistry.cpp
    llgl.cpp
    llglslshader.cpp
    llgltexture.cpp
    llimagegl.cpp
    llrender.cpp
    llrenderglmaterial.cpp
    llrendergltextureupload.cpp
    llrendergltonemap.cpp
    llrender2dutils.cpp
    llrendernavprim.cpp
    llrendersphere.cpp
    llrendertarget.cpp
    llshadermgr.cpp
    lltexture.cpp
    lltexturemanagerbridge.cpp
    lluiimage.cpp
    llvertexbuffer.cpp
    llglcommonfunc.cpp
    )
    
set(llrender_HEADER_FILES
    CMakeLists.txt

    llcubemap.h
    llcubemaparray.h
    llfontgl.h
    llfontvertexbuffer.h
    llfontfreetype.h
    llfontfreetypesvg.h
    llfontbitmapcache.h
    llfontregistry.h
    llgl.h
    llglheaders.h
    llglslshader.h
    llglstates.h
    llgltexture.h
    llgltypes.h
    llimagegl.h
    llrender.h
    llrenderglmaterial.h
    llrendergltextureupload.h
    llrendergltonemap.h
    llrender2dutils.h
    llrendernavprim.h
    llrendersphere.h
    llshadermgr.h
    lltexture.h
    lltexturemanagerbridge.h
    lluiimage.h
    lluiimage.inl
    llvertexbuffer.h
    llglcommonfunc.h
    )

list(APPEND llrender_SOURCE_FILES ${llrender_HEADER_FILES})

set(llrendercontract_SOURCE_FILES
    lldrawpacketcontract.cpp
    lldrawpacketcontract.h
    llmaterialcontract.cpp
    llmaterialcontract.h
    llmaterialdiagnostic.cpp
    llmaterialdiagnostic.h
    llrendercontract.cpp
    llrendercontract.h
    llshadermanifest.cpp
    llshadermanifest.h
    lltextureuploadcontract.cpp
    lltextureuploadcontract.h
    lltextureuploaddiagnostic.cpp
    lltextureuploaddiagnostic.h
    lltonemapcontract.cpp
    lltonemapcontract.h
    lltonemapdiagnostic.cpp
    lltonemapdiagnostic.h
    )

# Keep the backend-neutral contract independently linkable. In particular,
# this target must not inherit llrender's OpenGL or window dependencies.
add_library(llrendercontract ${llrendercontract_SOURCE_FILES})
target_include_directories(llrendercontract PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_HEADLESS)
  add_library (llrenderheadless
    ${llrender_SOURCE_FILES}
    )
  target_include_directories(llrenderheadless INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

  target_link_libraries(llrenderheadless
          llrendercontract
          llcommon
          llimage
          llmath
          llxml
          llfilesystem
          llwindowheadless
          ll::freetype
          OpenGL::GL
          )

  if (USE_PRECOMPILED_HEADERS)
    target_precompile_headers(llrenderheadless REUSE_FROM llprecompiled)
  endif()
endif (BUILD_HEADLESS)

if (LL_RENDER_BENCHMARK)
  set_property(SOURCE
      llglslshader.cpp
      llimagegl.cpp
      llshadermgr.cpp
      APPEND PROPERTY COMPILE_DEFINITIONS LL_RENDER_BENCHMARK=1)
  set_property(SOURCE
      llglslshader.cpp
      llimagegl.cpp
      llshadermgr.cpp
      PROPERTY SKIP_PRECOMPILE_HEADERS ON)
endif()

add_library (llrender ${llrender_SOURCE_FILES})
target_include_directories(llrender INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
target_link_libraries(llrender
        llrendercontract
        llcommon
        llimage
        llmath
        llfilesystem
        llxml
        llwindow
        ll::freetype
        )

if (NOT USE_SDL_WINDOW)
target_link_libraries(llrender
        OpenGL::GL
        )

endif()

if (USE_PRECOMPILED_HEADERS)
 target_precompile_headers(llrender REUSE_FROM llprecompiled)
endif ()

if (LL_TESTS)
  include(LLAddBuildTest)
  set_source_files_properties(
    tests/lldrawpacketcontract_test.cpp
    tests/llmaterialdiagnostic_test.cpp
    tests/llrendercontract_test.cpp
    tests/llshadermanifest_test.cpp
    tests/llrenderglmaterialregistry_test.cpp
    tests/llrendergltextureuploadregistry_test.cpp
    tests/llrendergltonemapregistry_test.cpp
    tests/lltextureuploadcontract_test.cpp
    tests/lltextureuploaddiagnostic_test.cpp
    tests/lltonemapdiagnostic_test.cpp
    PROPERTIES
    SKIP_PRECOMPILE_HEADERS TRUE
  )
  LL_ADD_INTEGRATION_TEST(lldrawpacketcontract "" "llrendercontract;llcommon")
  LL_ADD_INTEGRATION_TEST(llmaterialdiagnostic "" "llrendercontract;llcommon")
  LL_ADD_INTEGRATION_TEST(llrendercontract "" "llrendercontract;llcommon")
  LL_ADD_INTEGRATION_TEST(llshadermanifest "" "llrendercontract;llcommon")
  LL_ADD_INTEGRATION_TEST(llrenderglmaterialregistry "" "llrendercontract;llcommon")
  LL_ADD_INTEGRATION_TEST(llrendergltextureuploadregistry "" "llrender;llrendercontract;llcommon")
  LL_ADD_INTEGRATION_TEST(llrendergltonemapregistry "" "llrendercontract;llcommon")
  LL_ADD_INTEGRATION_TEST(lltextureuploadcontract "" "llrendercontract;llcommon")
  LL_ADD_INTEGRATION_TEST(lltextureuploaddiagnostic "" "llrendercontract;llcommon")
  LL_ADD_INTEGRATION_TEST(lltonemapdiagnostic "" "llrendercontract;llcommon")
endif ()

if (LL_VULKAN_TONEMAP_TEST)
  if (NOT LL_TESTS)
    message(FATAL_ERROR "LL_VULKAN_TONEMAP_TEST requires LL_TESTS=ON")
  endif ()

  if (NOT LINUX AND NOT DARWIN)
    message(FATAL_ERROR "LL_VULKAN_TONEMAP_TEST is supported only on Linux and macOS")
  endif ()

  find_package(Vulkan REQUIRED)
  find_program(LL_VULKAN_GLSLANG_VALIDATOR
      NAMES glslangValidator
      DOC "Path to glslangValidator for the Vulkan tonemap diagnostic")
  find_program(LL_VULKAN_SPIRV_VAL
      NAMES spirv-val
      DOC "Path to spirv-val for the Vulkan tonemap diagnostic")
  find_program(LL_VULKAN_SPIRV_CROSS
      NAMES spirv-cross
      DOC "Path to spirv-cross for Vulkan shader reflection")
  find_program(LL_VULKAN_SPIRV_DIS
      NAMES spirv-dis
      DOC "Path to spirv-dis for Vulkan shader decoration validation")

  if (NOT LL_VULKAN_GLSLANG_VALIDATOR OR
      NOT EXISTS "${LL_VULKAN_GLSLANG_VALIDATOR}")
    message(FATAL_ERROR
        "LL_VULKAN_TONEMAP_TEST requires glslangValidator; add it to PATH or set LL_VULKAN_GLSLANG_VALIDATOR")
  endif ()
  if (NOT LL_VULKAN_SPIRV_VAL OR NOT EXISTS "${LL_VULKAN_SPIRV_VAL}")
    message(FATAL_ERROR
        "LL_VULKAN_TONEMAP_TEST requires spirv-val; add it to PATH or set LL_VULKAN_SPIRV_VAL")
  endif ()
  if (NOT LL_VULKAN_SPIRV_CROSS OR NOT EXISTS "${LL_VULKAN_SPIRV_CROSS}")
    message(FATAL_ERROR
        "LL_VULKAN_TONEMAP_TEST requires spirv-cross; add it to PATH or set LL_VULKAN_SPIRV_CROSS")
  endif ()
  if (NOT LL_VULKAN_SPIRV_DIS OR NOT EXISTS "${LL_VULKAN_SPIRV_DIS}")
    message(FATAL_ERROR
        "LL_VULKAN_TONEMAP_TEST requires spirv-dis; add it to PATH or set LL_VULKAN_SPIRV_DIS")
  endif ()
  if (NOT LL_VULKAN_VALIDATION_LAYER_DIR OR
      NOT IS_DIRECTORY "${LL_VULKAN_VALIDATION_LAYER_DIR}" OR
      NOT EXISTS "${LL_VULKAN_VALIDATION_LAYER_DIR}/VkLayer_khronos_validation.json")
    message(FATAL_ERROR
        "LL_VULKAN_TONEMAP_TEST requires LL_VULKAN_VALIDATION_LAYER_DIR to contain VkLayer_khronos_validation.json")
  endif ()

  set(llvulkan_shader_source_dir "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders")
  set(llvulkan_shader_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/vulkan/shaders")
  set(llvulkan_vertex_source "${llvulkan_shader_source_dir}/tonemap.vert.glsl")
  set(llvulkan_fragment_source "${llvulkan_shader_source_dir}/tonemap.frag.glsl")
  set(llvulkan_vertex_spv "${llvulkan_shader_binary_dir}/tonemap.vert.spv")

  add_custom_command(
      OUTPUT "${llvulkan_vertex_spv}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${llvulkan_shader_binary_dir}"
      COMMAND "${CMAKE_COMMAND}" -E remove -f "${llvulkan_vertex_spv}.tmp"
      COMMAND "${LL_VULKAN_GLSLANG_VALIDATOR}"
              -V --target-env vulkan1.1 -S vert
              "-I${llvulkan_shader_source_dir}"
              -o "${llvulkan_vertex_spv}.tmp"
              "${llvulkan_vertex_source}"
      COMMAND "${LL_VULKAN_SPIRV_VAL}"
              --target-env vulkan1.1 "${llvulkan_vertex_spv}.tmp"
      COMMAND "${CMAKE_COMMAND}" -E rename
              "${llvulkan_vertex_spv}.tmp" "${llvulkan_vertex_spv}"
      DEPENDS
              "${llvulkan_vertex_source}"
              "${CMAKE_CURRENT_SOURCE_DIR}/../newview/app_settings/shaders/class1/deferred/postDeferredNoTCV.glsl"
      COMMENT "Compiling and validating Vulkan tonemap vertex shader"
      VERBATIM
  )

  set(llvulkan_fragment_spv_files)
  foreach (llvulkan_variant IN ITEMS 0 1 2 3 6 7)
    set(llvulkan_variant_defines)
    if (llvulkan_variant EQUAL 1 OR llvulkan_variant EQUAL 3 OR
        llvulkan_variant EQUAL 7)
      list(APPEND llvulkan_variant_defines -DLL_TONEMAP_NO_POST=1)
    endif ()
    if (llvulkan_variant EQUAL 2 OR llvulkan_variant EQUAL 3 OR
        llvulkan_variant EQUAL 6 OR llvulkan_variant EQUAL 7)
      list(APPEND llvulkan_variant_defines -DLL_TONEMAP_GAMMA_CORRECT=1)
    endif ()
    if (llvulkan_variant EQUAL 6 OR llvulkan_variant EQUAL 7)
      list(APPEND llvulkan_variant_defines -DLL_TONEMAP_LEGACY_GAMMA=1)
    endif ()

    set(llvulkan_fragment_spv
        "${llvulkan_shader_binary_dir}/tonemap.frag.${llvulkan_variant}.spv")
    add_custom_command(
        OUTPUT "${llvulkan_fragment_spv}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${llvulkan_shader_binary_dir}"
        COMMAND "${CMAKE_COMMAND}" -E remove -f "${llvulkan_fragment_spv}.tmp"
        COMMAND "${LL_VULKAN_GLSLANG_VALIDATOR}"
                -V --target-env vulkan1.1 -S frag
                "-I${llvulkan_shader_source_dir}"
                ${llvulkan_variant_defines}
                -o "${llvulkan_fragment_spv}.tmp"
                "${llvulkan_fragment_source}"
        COMMAND "${LL_VULKAN_SPIRV_VAL}"
                --target-env vulkan1.1 "${llvulkan_fragment_spv}.tmp"
        COMMAND "${CMAKE_COMMAND}" -E rename
                "${llvulkan_fragment_spv}.tmp" "${llvulkan_fragment_spv}"
        DEPENDS
                "${llvulkan_fragment_source}"
                "${CMAKE_CURRENT_SOURCE_DIR}/../newview/app_settings/shaders/class1/environment/srgbF.glsl"
                "${CMAKE_CURRENT_SOURCE_DIR}/../newview/app_settings/shaders/class1/deferred/tonemapUtilF.glsl"
                "${CMAKE_CURRENT_SOURCE_DIR}/../newview/app_settings/shaders/class1/deferred/postDeferredTonemap.glsl"
        COMMENT "Compiling and validating Vulkan tonemap fragment shader variant ${llvulkan_variant}"
        VERBATIM
    )
    list(APPEND llvulkan_fragment_spv_files "${llvulkan_fragment_spv}")
  endforeach ()

  add_custom_target(llvulkantonemap_shaders
      DEPENDS "${llvulkan_vertex_spv}" ${llvulkan_fragment_spv_files})
  set_target_properties(llvulkantonemap_shaders PROPERTIES FOLDER "Tests")

  set(llvulkan_material_vertex_source
      "${llvulkan_shader_source_dir}/material.vert.glsl")
  set(llvulkan_material_fragment_source
      "${llvulkan_shader_source_dir}/material.frag.glsl")

  add_executable(llshadermanifest_dump
      vulkan/llshadermanifest_dump.cpp)
  target_link_libraries(llshadermanifest_dump PRIVATE llrendercontract)
  set_target_properties(llshadermanifest_dump PROPERTIES FOLDER "Tests")

  set(llvulkan_material_reflection_verifier
      "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/verify_material_reflection.py")
  set(llvulkan_material_reflection_verifier_test
      "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/test_verify_material_reflection.py")
  set(llvulkan_material_hash_writer
      "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/write_shader_hashes.cmake")

  function(ll_add_vulkan_material_shader_profile profile artifact_stem target_name)
    set(profile_defines ${ARGN})
    set(artifact_path "${llvulkan_shader_binary_dir}/${artifact_stem}")
    set(vertex_spv "${artifact_path}.vert.spv")
    set(fragment_spv "${artifact_path}.frag.spv")
    set(vertex_reflection "${artifact_path}.vert.reflect.json")
    set(fragment_reflection "${artifact_path}.frag.reflect.json")
    set(vertex_disassembly "${artifact_path}.vert.spvasm")
    set(fragment_disassembly "${artifact_path}.frag.spvasm")
    set(manifest_expectation "${artifact_path}.manifest.json")
    set(module_hashes "${artifact_path}.modules.sha256")
    set(manifest_stamp "${artifact_path}.manifest.verified")

    add_custom_command(
        OUTPUT "${vertex_spv}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${llvulkan_shader_binary_dir}"
        COMMAND "${CMAKE_COMMAND}" -E remove -f "${vertex_spv}.tmp"
        COMMAND "${LL_VULKAN_GLSLANG_VALIDATOR}"
                -V --target-env vulkan1.1 -S vert
                "-I${llvulkan_shader_source_dir}"
                ${profile_defines}
                -o "${vertex_spv}.tmp"
                "${llvulkan_material_vertex_source}"
        COMMAND "${LL_VULKAN_SPIRV_VAL}"
                --target-env vulkan1.1 "${vertex_spv}.tmp"
        COMMAND "${CMAKE_COMMAND}" -E rename
                "${vertex_spv}.tmp"
                "${vertex_spv}"
        DEPENDS
                "${llvulkan_material_vertex_source}"
                "${CMAKE_CURRENT_SOURCE_DIR}/../newview/app_settings/shaders/class1/deferred/materialV.glsl"
        COMMENT "Compiling and validating ${profile} Vulkan material vertex shader"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${fragment_spv}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${llvulkan_shader_binary_dir}"
        COMMAND "${CMAKE_COMMAND}" -E remove -f "${fragment_spv}.tmp"
        COMMAND "${LL_VULKAN_GLSLANG_VALIDATOR}"
                -V --target-env vulkan1.1 -S frag
                "-I${llvulkan_shader_source_dir}"
                ${profile_defines}
                -o "${fragment_spv}.tmp"
                "${llvulkan_material_fragment_source}"
        COMMAND "${LL_VULKAN_SPIRV_VAL}"
                --target-env vulkan1.1 "${fragment_spv}.tmp"
        COMMAND "${CMAKE_COMMAND}" -E rename
                "${fragment_spv}.tmp"
                "${fragment_spv}"
        DEPENDS
                "${llvulkan_material_fragment_source}"
                "${CMAKE_CURRENT_SOURCE_DIR}/../newview/app_settings/shaders/class1/deferred/globalF.glsl"
                "${CMAKE_CURRENT_SOURCE_DIR}/../newview/app_settings/shaders/class3/deferred/materialF.glsl"
        COMMENT "Compiling and validating ${profile} Vulkan material fragment shader"
        VERBATIM
    )

    add_custom_command(
        OUTPUT
        "${manifest_stamp}"
        "${module_hashes}"
        BYPRODUCTS
        "${vertex_reflection}"
        "${fragment_reflection}"
        "${vertex_disassembly}"
        "${fragment_disassembly}"
        "${manifest_expectation}"
        COMMAND "${CMAKE_COMMAND}" -E remove -f
                "${manifest_stamp}"
                "${module_hashes}"
                "${vertex_reflection}"
                "${fragment_reflection}"
                "${vertex_disassembly}"
                "${fragment_disassembly}"
                "${manifest_expectation}"
        COMMAND "${LL_VULKAN_SPIRV_CROSS}"
                "${vertex_spv}"
                --reflect --output "${vertex_reflection}"
        COMMAND "${LL_VULKAN_SPIRV_CROSS}"
                "${fragment_spv}"
                --reflect --output "${fragment_reflection}"
        COMMAND "${LL_VULKAN_SPIRV_DIS}"
                "${vertex_spv}"
                -o "${vertex_disassembly}"
        COMMAND "${LL_VULKAN_SPIRV_DIS}"
                "${fragment_spv}"
                -o "${fragment_disassembly}"
        COMMAND "$<TARGET_FILE:llshadermanifest_dump>"
                --profile "${profile}"
                --output "${manifest_expectation}"
        COMMAND "${PYTHON_EXECUTABLE}" -B
                "${llvulkan_material_reflection_verifier_test}"
        COMMAND "${PYTHON_EXECUTABLE}"
                "${llvulkan_material_reflection_verifier}"
                --expectation "${manifest_expectation}"
                --vertex-reflection "${vertex_reflection}"
                --fragment-reflection "${fragment_reflection}"
                --vertex-disassembly "${vertex_disassembly}"
                --fragment-disassembly "${fragment_disassembly}"
        COMMAND "${CMAKE_COMMAND}"
                "-DVERTEX_MODULE=${vertex_spv}"
                "-DFRAGMENT_MODULE=${fragment_spv}"
                "-DOUTPUT_FILE=${module_hashes}"
                -P "${llvulkan_material_hash_writer}"
        COMMAND "${CMAKE_COMMAND}" -E touch
                "${manifest_stamp}"
        DEPENDS
        "${vertex_spv}"
        "${fragment_spv}"
        llshadermanifest_dump
        "${llvulkan_material_reflection_verifier}"
        "${llvulkan_material_reflection_verifier_test}"
        "${llvulkan_material_hash_writer}"
        COMMENT "Reflecting and validating the ${profile} Vulkan material manifest"
        VERBATIM
    )

    add_custom_target(${target_name}
        DEPENDS
        "${vertex_spv}"
        "${fragment_spv}"
        "${manifest_stamp}"
        "${module_hashes}")
    set_target_properties(${target_name} PROPERTIES FOLDER "Tests")
  endfunction()

  ll_add_vulkan_material_shader_profile(
      diagnostic material llvulkanmaterial_shaders)
  ll_add_vulkan_material_shader_profile(
      production material.production llvulkanmaterialproduction_shaders
      -DLL_VULKAN_MATERIAL_PRODUCTION=1)

  set(llvulkan_texture_upload_vertex_source
      "${llvulkan_shader_source_dir}/textureupload.vert.glsl")
  set(llvulkan_texture_upload_fragment_source
      "${llvulkan_shader_source_dir}/textureupload.frag.glsl")
  set(llvulkan_texture_upload_vertex_spv
      "${llvulkan_shader_binary_dir}/textureupload.vert.spv")
  set(llvulkan_texture_upload_fragment_spv
      "${llvulkan_shader_binary_dir}/textureupload.frag.spv")

  add_custom_command(
      OUTPUT "${llvulkan_texture_upload_vertex_spv}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${llvulkan_shader_binary_dir}"
      COMMAND "${CMAKE_COMMAND}" -E remove -f "${llvulkan_texture_upload_vertex_spv}.tmp"
      COMMAND "${LL_VULKAN_GLSLANG_VALIDATOR}"
              -V --target-env vulkan1.1 -S vert
              "-I${llvulkan_shader_source_dir}"
              -o "${llvulkan_texture_upload_vertex_spv}.tmp"
              "${llvulkan_texture_upload_vertex_source}"
      COMMAND "${LL_VULKAN_SPIRV_VAL}"
              --target-env vulkan1.1 "${llvulkan_texture_upload_vertex_spv}.tmp"
      COMMAND "${CMAKE_COMMAND}" -E rename
              "${llvulkan_texture_upload_vertex_spv}.tmp"
              "${llvulkan_texture_upload_vertex_spv}"
      DEPENDS
              "${llvulkan_texture_upload_vertex_source}"
              "${CMAKE_CURRENT_SOURCE_DIR}/../newview/app_settings/shaders/class1/interface/copyV.glsl"
      COMMENT "Compiling and validating Vulkan texture upload vertex shader"
      VERBATIM
  )

  add_custom_command(
      OUTPUT "${llvulkan_texture_upload_fragment_spv}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${llvulkan_shader_binary_dir}"
      COMMAND "${CMAKE_COMMAND}" -E remove -f "${llvulkan_texture_upload_fragment_spv}.tmp"
      COMMAND "${LL_VULKAN_GLSLANG_VALIDATOR}"
              -V --target-env vulkan1.1 -S frag
              "-I${llvulkan_shader_source_dir}"
              -o "${llvulkan_texture_upload_fragment_spv}.tmp"
              "${llvulkan_texture_upload_fragment_source}"
      COMMAND "${LL_VULKAN_SPIRV_VAL}"
              --target-env vulkan1.1 "${llvulkan_texture_upload_fragment_spv}.tmp"
      COMMAND "${CMAKE_COMMAND}" -E rename
              "${llvulkan_texture_upload_fragment_spv}.tmp"
              "${llvulkan_texture_upload_fragment_spv}"
      DEPENDS
              "${llvulkan_texture_upload_fragment_source}"
              "${CMAKE_CURRENT_SOURCE_DIR}/../newview/app_settings/shaders/class1/interface/copyF.glsl"
      COMMENT "Compiling and validating Vulkan texture upload fragment shader"
      VERBATIM
  )

  add_custom_target(llvulkantextureupload_shaders
      DEPENDS
      "${llvulkan_texture_upload_vertex_spv}"
      "${llvulkan_texture_upload_fragment_spv}")
  set_target_properties(llvulkantextureupload_shaders PROPERTIES FOLDER "Tests")

  set(llvulkan_link_library Vulkan::Vulkan)
  set(llvulkan_universal_macos OFF)
  if (DARWIN)
    if (NOT LL_VULKAN_MOLTENVK_ICD OR
        NOT EXISTS "${LL_VULKAN_MOLTENVK_ICD}")
      message(FATAL_ERROR
          "LL_VULKAN_TONEMAP_TEST on macOS requires LL_VULKAN_MOLTENVK_ICD to name a MoltenVK ICD manifest")
    endif ()

    list(FIND CMAKE_OSX_ARCHITECTURES "arm64" llvulkan_arm64_index)
    list(FIND CMAKE_OSX_ARCHITECTURES "x86_64" llvulkan_x86_64_index)
    if (NOT llvulkan_arm64_index EQUAL -1 AND
        NOT llvulkan_x86_64_index EQUAL -1)
      if (NOT LL_VULKAN_MACOS_X86_64_LOADER OR
          NOT EXISTS "${LL_VULKAN_MACOS_X86_64_LOADER}")
        message(FATAL_ERROR
            "A universal LL_VULKAN_TONEMAP_TEST requires LL_VULKAN_MACOS_X86_64_LOADER")
      endif ()
      if (NOT Vulkan_LIBRARY OR NOT EXISTS "${Vulkan_LIBRARY}")
        message(FATAL_ERROR
            "A universal LL_VULKAN_TONEMAP_TEST requires Vulkan_LIBRARY to name the host Vulkan loader dylib")
      endif ()
      find_program(LL_VULKAN_LIPO NAMES lipo DOC "Path to Apple's lipo tool")
      if (NOT LL_VULKAN_LIPO OR NOT EXISTS "${LL_VULKAN_LIPO}")
        message(FATAL_ERROR "A universal LL_VULKAN_TONEMAP_TEST requires lipo")
      endif ()
      find_program(LL_VULKAN_INSTALL_NAME_TOOL
          NAMES install_name_tool
          DOC "Path to Apple's install_name_tool")
      if (NOT LL_VULKAN_INSTALL_NAME_TOOL OR
          NOT EXISTS "${LL_VULKAN_INSTALL_NAME_TOOL}")
        message(FATAL_ERROR
            "A universal LL_VULKAN_TONEMAP_TEST requires install_name_tool")
      endif ()
      find_program(LL_VULKAN_CODESIGN
          NAMES codesign
          DOC "Path to Apple's codesign tool")
      if (NOT LL_VULKAN_CODESIGN OR NOT EXISTS "${LL_VULKAN_CODESIGN}")
        message(FATAL_ERROR
            "A universal LL_VULKAN_TONEMAP_TEST requires codesign")
      endif ()

      set(llvulkan_fat_loader_dir "${CMAKE_CURRENT_BINARY_DIR}/vulkan/lib")
      set(llvulkan_fat_loader "${llvulkan_fat_loader_dir}/libvulkan.1.dylib")
      add_custom_command(
          OUTPUT "${llvulkan_fat_loader}"
          COMMAND "${CMAKE_COMMAND}" -E make_directory "${llvulkan_fat_loader_dir}"
          COMMAND "${CMAKE_COMMAND}" -E remove -f "${llvulkan_fat_loader}.tmp"
          COMMAND "${LL_VULKAN_LIPO}" -create
                  "${Vulkan_LIBRARY}"
                  "${LL_VULKAN_MACOS_X86_64_LOADER}"
                  -output "${llvulkan_fat_loader}.tmp"
          COMMAND "${LL_VULKAN_INSTALL_NAME_TOOL}"
                  -id "@rpath/libvulkan.1.dylib"
                  "${llvulkan_fat_loader}.tmp"
          COMMAND "${LL_VULKAN_CODESIGN}" --force --sign -
                  "${llvulkan_fat_loader}.tmp"
          COMMAND "${CMAKE_COMMAND}" -E rename
                  "${llvulkan_fat_loader}.tmp" "${llvulkan_fat_loader}"
          DEPENDS "${Vulkan_LIBRARY}" "${LL_VULKAN_MACOS_X86_64_LOADER}"
          COMMENT "Creating universal Vulkan loader for the tonemap diagnostic"
          VERBATIM
      )
      add_custom_target(llvulkantonemap_loader DEPENDS "${llvulkan_fat_loader}")
      set_target_properties(llvulkantonemap_loader PROPERTIES FOLDER "Tests")
      set(llvulkan_link_library "${llvulkan_fat_loader}")
      set(llvulkan_universal_macos ON)
    endif ()
  endif ()

  add_library(llrendervulkantonemap STATIC
      vulkan/llrendervulkantonemap.cpp
      vulkan/llrendervulkantonemap.h
  )
  target_include_directories(llrendervulkantonemap
      PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vulkan")
  target_include_directories(llrendervulkantonemap SYSTEM
      PUBLIC "${Vulkan_INCLUDE_DIR}")
  if (llvulkan_universal_macos)
    add_dependencies(llrendervulkantonemap llvulkantonemap_loader)
  endif ()
  target_link_libraries(llrendervulkantonemap
      PUBLIC
      llrendercontract
      "${llvulkan_link_library}")
  set_target_properties(llrendervulkantonemap PROPERTIES FOLDER "Tests")

  add_executable(llvulkantonemap vulkan/llvulkantonemap_main.cpp)
  target_link_libraries(llvulkantonemap PRIVATE llrendervulkantonemap)
  add_dependencies(llvulkantonemap llvulkantonemap_shaders)

  add_executable(lltonemapcompare vulkan/lltonemapcompare_main.cpp)
  target_link_libraries(lltonemapcompare PRIVATE llrendercontract)

  add_library(llrendervulkanmaterial STATIC
      vulkan/llrendervulkanmaterial.cpp
      vulkan/llrendervulkanmaterial.h
  )
  target_include_directories(llrendervulkanmaterial
      PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vulkan")
  target_include_directories(llrendervulkanmaterial SYSTEM
      PUBLIC "${Vulkan_INCLUDE_DIR}")
  if (llvulkan_universal_macos)
    add_dependencies(llrendervulkanmaterial llvulkantonemap_loader)
  endif ()
  target_link_libraries(llrendervulkanmaterial
      PUBLIC
      llrendercontract
      "${llvulkan_link_library}")
  set_target_properties(llrendervulkanmaterial PROPERTIES FOLDER "Tests")

  add_executable(llvulkanmaterial vulkan/llvulkanmaterial_main.cpp)
  target_link_libraries(llvulkanmaterial PRIVATE llrendervulkanmaterial)
  add_dependencies(llvulkanmaterial llvulkanmaterial_shaders)

  add_executable(llmaterialcompare vulkan/llmaterialcompare_main.cpp)
  target_link_libraries(llmaterialcompare PRIVATE llrendercontract)

  add_library(llrendervulkantextureupload STATIC
      vulkan/llrendervulkantextureupload.cpp
      vulkan/llrendervulkantextureupload.h
  )
  target_include_directories(llrendervulkantextureupload
      PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vulkan")
  target_include_directories(llrendervulkantextureupload SYSTEM
      PUBLIC "${Vulkan_INCLUDE_DIR}")
  if (llvulkan_universal_macos)
    add_dependencies(llrendervulkantextureupload llvulkantonemap_loader)
  endif ()
  target_link_libraries(llrendervulkantextureupload
      PUBLIC
      llrendercontract
      "${llvulkan_link_library}")
  set_target_properties(llrendervulkantextureupload PROPERTIES FOLDER "Tests")

  add_executable(llvulkantextureupload vulkan/llvulkantextureupload_main.cpp)
  target_link_libraries(llvulkantextureupload PRIVATE llrendervulkantextureupload)
  add_dependencies(llvulkantextureupload llvulkantextureupload_shaders)

  add_executable(lltextureuploadcompare vulkan/lltextureuploadcompare_main.cpp)
  target_link_libraries(lltextureuploadcompare PRIVATE llrendercontract)

  if (DARWIN)
    set_target_properties(
        llvulkantonemap lltonemapcompare llvulkanmaterial llmaterialcompare
        llvulkantextureupload lltextureuploadcompare
        PROPERTIES
        OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "YES"
        FOLDER "Tests")
    if (llvulkan_universal_macos)
      set_target_properties(llvulkantonemap llvulkanmaterial llvulkantextureupload PROPERTIES
          BUILD_RPATH "${llvulkan_fat_loader_dir}")
    endif ()
  else ()
    set_target_properties(
        llvulkantonemap lltonemapcompare llvulkanmaterial llmaterialcompare
        llvulkantextureupload lltextureuploadcompare
        PROPERTIES
        FOLDER "Tests")
  endif ()

  set_source_files_properties(
      tests/llrendervulkanmaterialregistry_test.cpp
      tests/llrendervulkantextureuploadregistry_test.cpp
      tests/llrendervulkantonemapregistry_test.cpp
      PROPERTIES
      SKIP_PRECOMPILE_HEADERS TRUE
  )
  LL_ADD_INTEGRATION_TEST(
      llrendervulkantonemapregistry
      ""
      "llrendervulkantonemap;${llvulkan_link_library};llrendercontract;llcommon")
  LL_ADD_INTEGRATION_TEST(
      llrendervulkanmaterialregistry
      ""
      "llrendervulkanmaterial;${llvulkan_link_library};llrendercontract;llcommon")
  LL_ADD_INTEGRATION_TEST(
      llrendervulkantextureuploadregistry
      ""
      "llrendervulkantextureupload;${llvulkan_link_library};llrendercontract;llcommon")
  if (llvulkan_universal_macos)
    set_target_properties(
        INTEGRATION_TEST_llrendervulkantonemapregistry
        INTEGRATION_TEST_llrendervulkanmaterialregistry
        INTEGRATION_TEST_llrendervulkantextureuploadregistry
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH OFF
        BUILD_RPATH "${llvulkan_fat_loader_dir}")
  endif ()

  add_dependencies(
      BUILD_TESTS
      llvulkantonemap
      lltonemapcompare
      llvulkanmaterial
      llmaterialcompare
      llvulkantextureupload
      lltextureuploadcompare)
endif ()
