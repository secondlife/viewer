# -*- cmake -*-
include(Prebuilt)

if (WINDOWS OR LINUX)
  use_prebuilt_binary(glext)
endif (WINDOWS OR LINUX)

use_prebuilt_binary(glh_linear)
