# -*- cmake -*-

# The copy_win_scripts folder contains scripts handy for launching the 
# from the windows command line on windows.
# The cmake target created copies the scripts to the 
# build directory being used, which where the scripts 
# need to be executed from.

include(CMakeCopyIfDifferent)

set(win_scripts-src ${CMAKE_SOURCE_DIR}/copy_win_scripts)
set(win_scripts-dst ${CMAKE_BINARY_DIR}/batch)

set(file-list
    llstart.py 
    start-client.py 
    start-servers.py 
    stop-servers.py
    user_config.py
    )

foreach(file ${file-list})
    if(EXISTS ${win_scripts-src}/${file})    
        set(win_scripts-files ${win_scripts-files} ${file})
    endif(EXISTS ${win_scripts-src}/${file})
endforeach(file ${file-list})
    
copy_if_different(
    ${win_scripts-src} 
    ${win_scripts-dst} 
    win_scripts-targets 
    ${win_scripts-files} 
    )
    
add_custom_target(copy_win_scripts ALL DEPENDS ${win_scripts-targets})
