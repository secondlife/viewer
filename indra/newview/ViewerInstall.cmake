install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${VIEWER_BINARY_NAME}
        DESTINATION ${APP_BINARY_DIR}
        )

install(DIRECTORY skins app_settings linux_tools
        DESTINATION ${APP_SHARE_DIR}
        PATTERN ".svn" EXCLUDE
        )

set(VULKAN_MATERIAL_ARTIFACT_INSTALL_DIR
    "${APP_SHARE_DIR}/app_settings/shaders/vulkan/legacy_normspec")
install(CODE
    "set(VULKAN_MATERIAL_INSTALL_DESTINATION \"\$ENV{DESTDIR}${VULKAN_MATERIAL_ARTIFACT_INSTALL_DIR}\")\ninclude(\"${CMAKE_CURRENT_SOURCE_DIR}/clean_vulkan_material_install.cmake\")")
if (LL_VULKAN_TONEMAP_TEST)
  install(FILES
      "${LL_VULKAN_MATERIAL_PRODUCTION_ARTIFACT_DIR}/material.production.vert.spv"
      DESTINATION "${VULKAN_MATERIAL_ARTIFACT_INSTALL_DIR}"
      RENAME production.vert.spv)
  install(FILES
      "${LL_VULKAN_MATERIAL_PRODUCTION_ARTIFACT_DIR}/material.production.frag.spv"
      DESTINATION "${VULKAN_MATERIAL_ARTIFACT_INSTALL_DIR}"
      RENAME production.frag.spv)
endif ()

find_file(IS_ARTWORK_PRESENT NAMES have_artwork_bundle.marker
          PATHS ${VIEWER_DIR}/newview/res)

if (IS_ARTWORK_PRESENT)
  install(DIRECTORY res res-sdl character
          DESTINATION ${APP_SHARE_DIR}
          PATTERN ".svn" EXCLUDE
          )
else (IS_ARTWORK_PRESENT)
  message(STATUS "WARNING: Artwork is not present, and will not be installed")
endif (IS_ARTWORK_PRESENT)

install(FILES featuretable_linux.txt
        DESTINATION ${APP_SHARE_DIR}
        )

install(FILES ${SCRIPTS_DIR}/messages/message_template.msg
        DESTINATION ${APP_SHARE_DIR}/app_settings
        )
