# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  include(FindPkgConfig)

  pkg_check_modules(PULSEAUDIO REQUIRED libpulse-mainloop-glib)

elseif (LINUX)
  use_prebuilt_binary(pulseaudio)
  set(PULSEAUDIO_FOUND ON FORCE BOOL)
  set(PULSEAUDIO_INCLUDE_DIRS
      ${LIBS_PREBUILT_DIR}/include
      )
  # We don't need to explicitly link against pulseaudio itself, because
  # the viewer probes for the system's copy at runtime.
  set(PULSEAUDIO_LIBRARIES
    # none needed!
    )
endif (STANDALONE)

if (PULSEAUDIO_FOUND)
  set(PULSEAUDIO ON CACHE BOOL "Build with PulseAudio support, if available.")
endif (PULSEAUDIO_FOUND)

if (PULSEAUDIO)
  add_definitions(-DLL_PULSEAUDIO_ENABLED=1)
endif (PULSEAUDIO)
