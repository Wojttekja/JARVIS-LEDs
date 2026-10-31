add_executable(02_serial main.c)
target_link_libraries(02_serial pico_stdlib pico_unique_id)
pico_enable_stdio_usb(02_serial 1)   # printf over USB CDC
pico_enable_stdio_uart(02_serial 0)
pico_add_extra_outputs(02_serial)
