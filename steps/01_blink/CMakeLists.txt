add_executable(01_blink main.c)
target_link_libraries(01_blink pico_stdlib)
pico_add_extra_outputs(01_blink)  # produces 01_blink.uf2
