cmake_minimum_required(VERSION 3.22.0)

include(${SdkRootDirPath}/cmake/extension/mcux.cmake)

project(oscilloscope_secondary
    LANGUAGES C CXX ASM
    PROJECT_BOARD_PORT_PATH
        ${board_root}/${board}/multicore_examples/multicore_manager
)

include(${SdkRootDirPath}/CMakeLists.txt)

include(
    ${SdkRootDirPath}/${board_root}/${board}/multicore_examples/reconfig.cmake
    OPTIONAL
)
include(
    ${SdkRootDirPath}/${board_root}/${board}/multicore_examples/multicore_manager/${core_id}/reconfig.cmake
    OPTIONAL
)

mcux_add_source(
    SOURCES
        main.c
        FreeRTOSConfig.h
        osc_adc.c
        osc_adc.h
        osc_dac_sine.c
        osc_dac_sine.h
        osc_fft.c
        osc_fft.h
        ../shared/osc_shared.c
        ../shared/osc_shared.h
)

mcux_add_include(
    BASE_PATH ${SdkRootDirPath}
    INCLUDES
        ${board_root}/${board}/freertos_config_board/${core_id}
)

mcux_add_include(
    INCLUDES
        .
        ../shared
)

mcux_add_configuration(
    CC "-D__USE_SHMEM -DMCMGR_HANDLE_EXCEPTIONS -D__SEMIHOST_HARDFAULT_DISABLE"
)

mcux_add_linker_symbol(
    SYMBOLS
        "__use_shmem__=1"
        "__multicore__=1"
)

mcux_convert_binary(
    TOOLCHAINS armgcc mdk iar
    BINARY ${APPLICATION_BINARY_DIR}/${CONFIG_TOOLCHAIN}/core1_image.bin
)
