set(CAN2040_SOURCE can2040/src)
set(CBUSDEFS cbusdefs)
set(CBUSPICOLIB CBUSPicoLib)
set(INIH inih)

add_subdirectory(FatFsPicoSPI/FatFs_SPI build)

add_executable(CBUSPico
   # Source for CAN2040 PIO based CAN controller
   ${CAN2040_SOURCE}/can2040.c
   # Sources for CBUSPico libary 
   ${CBUSPICOLIB}/CBUSLED.cpp
   ${CBUSPICOLIB}/CBUSSwitch.cpp
   ${CBUSPICOLIB}/CBUSConfig.cpp
   ${CBUSPICOLIB}/CBUSParams.cpp
   ${CBUSPICOLIB}/SystemTick.cpp
   ${CBUSPICOLIB}/CBUSCircularBuffer.cpp
   ${CBUSPICOLIB}/CBUSLongMessage.cpp
   ${CBUSPICOLIB}/GridConnectPicoW/CBUSGridConnect.cpp
   ${CBUSPICOLIB}/CBUS.cpp
   ${CBUSPICOLIB}/ACAN2040.cpp
   ${CBUSPICOLIB}/CBUSACAN2040.cpp
   # PICOWi module using libary
   hw_config.c
   ${INIH}/ini.c
   CBUSWiFi.cpp
   CANPicoWi.cpp
)

target_include_directories(CBUSPico PRIVATE
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CAN2040_SOURCE}
   ${CBUSDEFS}
   ${CBUSPICOLIB}
   ${CBUSPICOLIB}/GridConnectPicoW
   ${INIH}
)

target_compile_options(CBUSPico PRIVATE
   # can2040 has an unused param, -Wpedantic is a step too far !
   -Wall -Wextra -Werror -Wno-unused-parameter 
)

# Custom linker scipt to put CAN2040 code into RAM
pico_set_linker_script(CBUSPico ${CMAKE_CURRENT_SOURCE_DIR}/memmap_canpiwi.ld)

target_link_libraries(
   CBUSPico
   pico_stdlib
   # pico_stdio_semihosting
   pico_multicore
   cmsis_core
   hardware_i2c
   hardware_flash
   pico_cyw43_arch_lwip_threadsafe_background
   pico_lwip_http
   FatFs_SPI
)

pico_add_extra_outputs(CBUSPico)
