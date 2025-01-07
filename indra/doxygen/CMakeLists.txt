# -*- cmake -*-

set(ROOT_PROJECT_NAME "SecondLife" CACHE STRING
    "The root project/makefile/solution name. Defaults to SecondLife.")
project(${ROOT_PROJECT_NAME})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(Variables)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    GET_FILENAME_COMPONENT(DOXYGEN_TOP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.. PATH)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
                      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/..
                      COMMENT "Generating API documentation with Doxygen" VERBATIM
                      )
endif(DOXYGEN_FOUND)

