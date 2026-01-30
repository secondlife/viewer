# -*- cmake -*-
include_guard()

add_library(ll::hunspell INTERFACE IMPORTED)

find_package(PkgConfig REQUIRED)

pkg_check_modules(hunspell REQUIRED IMPORTED_TARGET GLOBAL hunspell)
target_link_libraries(ll::hunspell INTERFACE PkgConfig::hunspell)
