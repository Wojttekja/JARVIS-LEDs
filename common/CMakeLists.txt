add_library(pico_leds_common STATIC
    board_id.c
    color.c
    flash_store.c
    nrf24.c
    protocol.c
)
target_include_directories(pico_leds_common PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(pico_leds_common PUBLIC
    pico_stdlib pico_unique_id hardware_spi hardware_flash pico_flash
)
