cmake_minimum_required(VERSION 3.22.0)

include(${SdkRootDirPath}/cmake/extension/mcux.cmake)

project(oscilloscope
    LANGUAGES C CXX ASM
    PROJECT_BOARD_PORT_PATH
        ${board_root}/${board}/lvgl_examples/lvgl_guider_bm
)

include(${SdkRootDirPath}/CMakeLists.txt)

mcux_add_source(
    SOURCES
        ./main.c
        ./osc_core1_boot.c
        ./osc_core1_boot.h
        ./shared/osc_shared.c
        ./shared/osc_shared.h
        ./ui/app_ui.c
        ./ui/app_ui.h
        ./ui/ui_i18n.c
        ./ui/ui_i18n.h
        ./ui/ui_scope_view.c
        ./ui/ui_scope_view.h
        ./ui/ui_theme.c
        ./ui/ui_theme.h
        ./ui/fonts/osc_font_zh_16.c
        ./ui/fonts/osc_font_zh_24.c
)

mcux_add_include(
    INCLUDES
        .
        ./shared
        ./ui
)

mcux_add_configuration(
    CC "-DCORE1_IMAGE_COPY_TO_RAM"
)

mcux_add_linker_symbol(
    SYMBOLS
        "__use_shmem__=1"
        "__multicore__=1"
)

mcux_add_include(
    TOOLCHAINS mdk armgcc
    BASE_PATH ${APPLICATION_BINARY_DIR}
    INCLUDES ../oscilloscope_secondary/${CONFIG_TOOLCHAIN}/
)

get_filename_component(
    OSC_CORE1_IMAGE
    "${APPLICATION_BINARY_DIR}/../oscilloscope_secondary/${CONFIG_TOOLCHAIN}/core1_image.bin"
    ABSOLUTE
)
set_property(
    SOURCE ${SdkRootDirPath}/components/misc_utilities/fsl_incbin.S
    APPEND
    PROPERTY OBJECT_DEPENDS "${OSC_CORE1_IMAGE}"
)

include(
    ${SdkRootDirPath}/${board_root}/${board}/lvgl_examples/lvgl_guider_bm/reconfig.cmake
    OPTIONAL
)
include(
    ${SdkRootDirPath}/${board_root}/${board}/multicore_examples/reconfig.cmake
    OPTIONAL
)

mcux_add_linker_symbol(
    SYMBOLS
        "__stack_size__=0x4000"
)

mcux_convert_binary(BINARY ${APPLICATION_BINARY_DIR}/${MCUX_SDK_PROJECT_NAME}.bin)
