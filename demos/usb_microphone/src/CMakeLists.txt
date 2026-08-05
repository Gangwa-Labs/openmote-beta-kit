# Main (app) component for the OpenMote USB microphone.
# REQUIRES: driver         -> esp_driver_gpio (driver/gpio.h) + esp_driver_i2s (driver/i2s_pdm.h)
#           usb_device_uac -> the turnkey UAC device (usb_device_uac.h)
file(GLOB app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "." "../include"
    REQUIRES driver usb_device_uac
)
