# -*- cmake -*-
# Copies a binary back to the source directory

MACRO(COPY_BACK_TO_SOURCE target)
   GET_TARGET_PROPERTY(FROM ${target} LOCATION)
   SET(TO ${CMAKE_CURRENT_SOURCE_DIR})
   #MESSAGE("TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${FROM} ${TO}")
   ADD_CUSTOM_COMMAND(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${FROM} ${TO}
        DEPENDS ${FROM}
        COMMENT "Copying ${target} to ${CMAKE_CURRENT_BINARY_DIR}"
        )
ENDMACRO(COPY_BACK_TO_SOURCE)


