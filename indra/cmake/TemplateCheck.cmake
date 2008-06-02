# -*- cmake -*-

include(Python)

macro (check_message_template _target)
  add_custom_command(
      TARGET ${_target}
      PRE_LINK
      COMMAND ${PYTHON_EXECUTABLE}
      ARGS ${SCRIPTS_DIR}/template_verifier.py
           --mode=development --cache_master
      COMMENT "Verifying message template"
      )
endmacro (check_message_template)
