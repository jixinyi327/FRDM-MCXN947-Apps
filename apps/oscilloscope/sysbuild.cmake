ExternalMCUXProject_Add(
    APPLICATION oscilloscope_secondary
    SOURCE_DIR ${APP_DIR}/secondary
    board ${SB_CONFIG_secondary_board}
    core_id ${SB_CONFIG_secondary_core_id}
    config ${SB_CONFIG_secondary_config}
    toolchain ${SB_CONFIG_secondary_toolchain}
)

add_dependencies(${DEFAULT_IMAGE} oscilloscope_secondary)
