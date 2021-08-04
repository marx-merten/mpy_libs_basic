# Create an INTERFACE library for our CPP module.
add_library(usermod_haldebug INTERFACE)

# Add our source files to the library.
target_sources(usermod_haldebug INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/module.c

    ${CMAKE_CURRENT_LIST_DIR}/mod_led.c
    ${CMAKE_CURRENT_LIST_DIR}/mod_led_utils.c
    ${CMAKE_CURRENT_LIST_DIR}/mod_led_colorutils.c
    ${CMAKE_CURRENT_LIST_DIR}/led/rmt_hal.c
)

# Add the current directory as an include directory.
target_include_directories(usermod_haldebug INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}

)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_haldebug)
