if (NOT DEFINED VULKAN_MATERIAL_INSTALL_DESTINATION OR
    VULKAN_MATERIAL_INSTALL_DESTINATION STREQUAL "")
  message(FATAL_ERROR "VULKAN_MATERIAL_INSTALL_DESTINATION is required")
endif ()

# Walk existing parents without following them. Recursive removal is safe only
# when the dedicated destination sits below real install-tree directories.
get_filename_component(
    _vulkan_material_install_parent
    "${VULKAN_MATERIAL_INSTALL_DESTINATION}"
    DIRECTORY)
while (TRUE)
  if (IS_SYMLINK "${_vulkan_material_install_parent}")
    message(FATAL_ERROR
        "Refusing Vulkan material cleanup through symlink: ${_vulkan_material_install_parent}")
  endif ()
  get_filename_component(
      _vulkan_material_install_next_parent
      "${_vulkan_material_install_parent}"
      DIRECTORY)
  if (_vulkan_material_install_next_parent STREQUAL
      _vulkan_material_install_parent)
    break()
  endif ()
  set(_vulkan_material_install_parent
      "${_vulkan_material_install_next_parent}")
endwhile ()

if (IS_SYMLINK "${VULKAN_MATERIAL_INSTALL_DESTINATION}")
  file(REMOVE "${VULKAN_MATERIAL_INSTALL_DESTINATION}")
elseif (EXISTS "${VULKAN_MATERIAL_INSTALL_DESTINATION}")
  if (IS_DIRECTORY "${VULKAN_MATERIAL_INSTALL_DESTINATION}")
    file(REMOVE_RECURSE "${VULKAN_MATERIAL_INSTALL_DESTINATION}")
  else ()
    file(REMOVE "${VULKAN_MATERIAL_INSTALL_DESTINATION}")
  endif ()
endif ()

unset(_vulkan_material_install_next_parent)
unset(_vulkan_material_install_parent)
