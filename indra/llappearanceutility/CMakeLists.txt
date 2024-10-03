# -*- cmake -*-

project(appearance_utility)

include(00-Common)
include(Boost)
include(LLAppearance)
include(LLCommon)
include(LLImage)
include(LLMath)
include(LLWindow)
include(Linking)
#include(GooglePerfTools)


set(appearance_utility_SOURCE_FILES
    appearance_utility.cpp
    llappappearanceutility.cpp
    llbakingavatar.cpp
    llbakingjoint.cpp
    llbakingjointmesh.cpp
    llbakingprocess.cpp
    llbakingshadermgr.cpp
    llbakingtexlayer.cpp
    llbakingtexture.cpp
    llbakingwearable.cpp
    llbakingwearablesdata.cpp
    llbakingwindow.cpp
    llprocessparams.cpp
    llprocessskin.cpp
    llprocesstexture.cpp
    )

set(appearance_utility_HEADER_FILES
    CMakeLists.txt
    llappappearanceutility.h
    llbakingavatar.h
    llbakingjoint.h
    llbakingjointmesh.h
    llbakingprocess.h
    llbakingshadermgr.h
    llbakingtexlayer.h
    llbakingtexture.h
    llbakingwearable.h
    llbakingwearablesdata.h
    llbakingwindow.h
    llprocessparams.h
    llprocessskin.h
    llprocesstexture.h
    )

set_source_files_properties(${appearance_utility_HEADER_FILES}
                            PROPERTIES HEADER_FILES_ONLY TRUE)

list(APPEND appearance_utility_SOURCE_FILES ${appearance_utility_HEADER_FILES})

add_executable(appearance-utility-bin ${appearance_utility_SOURCE_FILES})

target_link_libraries(appearance-utility-bin
    llcharacter
    llinventory
    llappearance
    llwindow
    llrender
    llxml
    )

add_custom_target(appearance-utility-bin-target ALL
                  DEPENDS appearance-utility-bin)


if (BUILD_HEADLESS)
  add_executable(appearance-utility-headless-bin ${appearance_utility_SOURCE_FILES})
  
  target_link_libraries(appearance-utility-headless-bin
      ${LLAPPEARANCE_HEADLESS_LIBRARIES}
      ${LLWINDOW_HEADLESS_LIBRARIES}
      ${BOOST_COROUTINE_LIBRARY}
      ${BOOST_CONTEXT_LIBRARY}
      ${BOOST_SYSTEM_LIBRARY}
      ${TCMALLOC_LIBRARIES}
      )
  
  add_custom_target(appearance-utility-headless-bin-target ALL
                    DEPENDS appearance-utility-headless-bin)
endif (BUILD_HEADLESS)

