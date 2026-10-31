add_executable(05_inputs
    main.c
    ${CMAKE_SOURCE_DIR}/remote/input.c
)
target_include_directories(05_inputs PRIVATE ${CMAKE_SOURCE_DIR}/remote)
target_link_libraries(05_inputs pico_stdlib pico_leds_common)
pico_enable_stdio_usb(05_inputs 1)
pico_enable_stdio_uart(05_inputs 0)
pico_add_extra_outputs(05_inputs)
