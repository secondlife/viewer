# -*- cmake -*-

project(llmeshoptimizer)

include(MESHOPTIMIZER)

include(00-Common)
include(LLCommon)
include(LLMath)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESHOPTIMIZER_INCLUDE_DIR}
    ${MESHOPTIMIZER_INCLUDE_DIRS}
    ${LIBS_PREBUILT_DIR}/include  #access to boost headers, needed for LLError
    )

set(llmeshoptimizer_SOURCE_FILES
    llmeshoptimizer.cpp
    )

set(llmeshoptimizer_HEADER_FILES
    CMakeLists.txt

    llmeshoptimizer.h
    )

set_source_files_properties(${llmeshoptimizer_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llmeshoptimizer_SOURCE_FILES ${llmeshoptimizer_HEADER_FILES})

#if (USE_MESHOPT)
  add_library (llmeshoptimizer ${llmeshoptimizer_SOURCE_FILES})

  target_link_libraries(llmeshoptimizer
    ${LLCOMMON_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${MESHOPTIMIZER_LIBRARIES})
  
  # Add tests

#endif (USE_MESHOPT)
