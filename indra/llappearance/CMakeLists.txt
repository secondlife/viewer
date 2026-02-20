# -*- cmake -*-

set(llappearance_SOURCE_FILES
    llavatarappearance.cpp
    llavatarjoint.cpp
    llavatarjointmesh.cpp
    lldriverparam.cpp
    lljointdata.h
    lllocaltextureobject.cpp
    llpolyskeletaldistortion.cpp
    llpolymesh.cpp
    llpolymorph.cpp
    lltexglobalcolor.cpp
    lltexlayer.cpp
    lltexlayerparams.cpp
    llwearable.cpp
    llwearabledata.cpp
    llwearabletype.cpp
    llviewervisualparam.cpp
    llavatarappearancedefines.cpp
    )

set(llappearance_HEADER_FILES
    llavatarappearance.h
    llavatarjoint.h
    llavatarjointmesh.h
    lldriverparam.h
    lljointpickname.h
    lllocaltextureobject.h
    llpolyskeletaldistortion.h
    llpolymesh.h
    llpolymorph.h
    lltexglobalcolor.h
    lltexlayer.h
    lltexlayerparams.h
    llwearable.h
    llwearabledata.h
    llwearabletype.h
    llviewervisualparam.h
    llavatarappearancedefines.h
    )

list(APPEND llappearance_SOURCE_FILES ${llappearance_HEADER_FILES})

add_library (llappearance ${llappearance_SOURCE_FILES})

target_link_libraries(llappearance
        PUBLIC
        llcharacter
        llinventory
        llimage
        llrender
        llfilesystem
        llmath
        llxml
        llmessage
        llcorehttp
        llcommon
    )
target_include_directories( llappearance  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(llappearance REUSE_FROM llprecompiled)

if (BUILD_HEADLESS)
  add_library (llappearanceheadless ${llappearance_SOURCE_FILES})
  target_include_directories( llappearanceheadless  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

  target_link_libraries(llappearanceheadless
          PUBLIC
          llcharacter
          llinventory
          llimage
          llrenderheadless
          llfilesystem
          llmath
          llxml
          llmessage
          llcorehttp
          llcommon
      )

  target_precompile_headers(llappearanceheadless REUSE_FROM llprecompiled)
endif (BUILD_HEADLESS)
