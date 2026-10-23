cmake_minimum_required(VERSION 3.22.0)

include(${SdkRootDirPath}/cmake/extension/mcux.cmake)

project(touchscreen_tester
    LANGUAGES C CXX ASM
    PROJECT_BOARD_PORT_PATH
        ${board_root}/${board}/lvgl_examples/lvgl_guider_bm
)

include(${SdkRootDirPath}/CMakeLists.txt)

mcux_add_include(
    INCLUDES
        ./ui
)

mcux_add_source(
    SOURCES
        ./main.c
        ./ui/app_ui.c
        ./ui/app_ui.h
        ./ui/screen_accuracy.c
        ./ui/screen_accuracy.h
        ./ui/screen_block_check.c
        ./ui/screen_block_check.h
        ./ui/screen_free_draw.c
        ./ui/screen_free_draw.h
        ./ui/screen_settings.c
        ./ui/screen_settings.h
        ./ui/ui_common.c
        ./ui/ui_common.h
        ./ui/ui_theme.c
        ./ui/ui_theme.h
)

include(
    ${SdkRootDirPath}/${board_root}/${board}/lvgl_examples/lvgl_guider_bm/reconfig.cmake
    OPTIONAL
)

mcux_convert_binary(BINARY ${APPLICATION_BINARY_DIR}/${MCUX_SDK_PROJECT_NAME}.bin)
