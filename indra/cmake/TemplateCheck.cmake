# -*- cmake -*-

include(Python)

macro (check_message_template _target)
  add_custom_command(
      TARGET ${_target}
      POST_BUILD
      COMMAND ${PYTHON_EXECUTABLE}
      ARGS ${SCRIPTS_DIR}/md5check.py
           3f19d130400c547de36278a6b6f9b028
           ${SCRIPTS_DIR}/messages/message_template.msg
      COMMENT "Verifying message template - See http://wiki.secondlife.com/wiki/Template_verifier.py"
      )
endmacro (check_message_template)
