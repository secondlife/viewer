set(WATCHDOG_DIR "${CMAKE_CURRENT_BINARY_DIR}")

if (LL_WINDOWS)
    set(WATCHDOG_EXE "${WATCHDOG_DIR}/watchdog.exe")
    set(wswitch "-w")
else ()
    set(WATCHDOG_EXE "${WATCHDOG_DIR}/watchdog")
    set(wswitch "")
endif()

    add_custom_command(
      OUTPUT  "${WATCHDOG_EXE}"
      COMMAND "pyinstaller"
      ARGS
        # suppress opening console window at runtime
        ${wswitch}
        # don't prompt, just overwrite previous exe
        -y
        # don't reuse previous PyInstaller cache
        --clean
        # cram everything into single .exe file
        --onefile
        # control build-time output detail (for CI builds)
        --log-level DEBUG
##      # search specified path for Python imports
##      -p dirname
        # where to put the generated executable
        --distpath "${WATCHDOG_DIR}"
        # script to compile
        ${CMAKE_CURRENT_SOURCE_DIR}/watchdog.py
      DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/watchdog.py
      COMMENT "Building watchdog executable"
      )
    add_custom_target(watchdog_exe DEPENDS "${WATCHDOG_EXE}")
