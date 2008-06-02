install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/secondlife-bin
        DESTINATION ${APP_BINARY_DIR}
        )

install(DIRECTORY skins app_settings linux_tools
        DESTINATION ${APP_SHARE_DIR}
        PATTERN ".svn" EXCLUDE
        )

find_file(IS_ARTWORK_PRESENT NAMES avatar_lad.xml
          PATHS ${VIEWER_DIR}/newview/character)

if (IS_ARTWORK_PRESENT)
  install(DIRECTORY res res-sdl character
          DESTINATION ${APP_SHARE_DIR}
          PATTERN ".svn" EXCLUDE
          )
else (IS_ARTWORK_PRESENT)
  message(STATUS "WARNING: Artwork is not present, and will not be installed")
endif (IS_ARTWORK_PRESENT)

install(FILES featuretable_linux.txt featuretable_solaris.txt gpu_table.txt
        DESTINATION ${APP_SHARE_DIR}
        )

install(FILES ${SCRIPTS_DIR}/messages/message_template.msg
        DESTINATION ${APP_SHARE_DIR}/app_settings
        )
