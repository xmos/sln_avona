set(FFD_SRC_ROOT ${CMAKE_CURRENT_LIST_DIR})

set(MODEL_LANGUAGE "english_usa")
#set(CYBERON_COMMAND_NET_FILE "${FFD_SRC_ROOT}/model/english_usa/Hello_XMOS_pack_WithTxt.bin.Enc.NibbleSwap")
set(CYBERON_COMMAND_NET_FILE "${FFD_SRC_ROOT}/model/english_usa/Testing_level_2_pack_WithTxt.bin.Enc.NibbleSwap")

#**********************
# Gather Sources
#**********************
file(GLOB_RECURSE APP_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.c )


set(APP_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/gpio_ctrl
    ${CMAKE_CURRENT_LIST_DIR}/src/intent_engine
    ${CMAKE_CURRENT_LIST_DIR}/src/power
)
set(RTOS_CONF_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/src/rtos_conf
)

#**********************
# QSPI Flash Layout
#**********************
set(BOOT_PARTITION_SIZE 0x100000)
set(FILESYSTEM_SIZE_KB 1024)
math(EXPR FILESYSTEM_SIZE_BYTES
     "1024 * ${FILESYSTEM_SIZE_KB}"
     OUTPUT_FORMAT HEXADECIMAL
)

set(CALIBRATION_PATTERN_START_ADDRESS ${BOOT_PARTITION_SIZE})

math(EXPR FILESYSTEM_START_ADDRESS
    "${CALIBRATION_PATTERN_START_ADDRESS} + ${LIB_QSPI_FAST_READ_DEFAULT_CAL_SIZE_BYTES}"
    OUTPUT_FORMAT HEXADECIMAL
)

math(EXPR MODEL_START_ADDRESS
    "${FILESYSTEM_START_ADDRESS} + ${FILESYSTEM_SIZE_BYTES}"
    OUTPUT_FORMAT HEXADECIMAL
)

set(CALIBRATION_PATTERN_DATA_PARTITION_OFFSET 0)

math(EXPR FILESYSTEM_DATA_PARTITION_OFFSET
    "${CALIBRATION_PATTERN_DATA_PARTITION_OFFSET} + ${LIB_QSPI_FAST_READ_DEFAULT_CAL_SIZE_BYTES}"
    OUTPUT_FORMAT DECIMAL
)

math(EXPR MODEL_DATA_PARTITION_OFFSET
    "${FILESYSTEM_DATA_PARTITION_OFFSET} + ${FILESYSTEM_SIZE_BYTES}"
    OUTPUT_FORMAT DECIMAL
)


#**********************
# Flags
#**********************
set(APP_COMPILER_FLAGS
    -Os
    -g
    -report
    -fxscope
    -mcmodel=large
    -Wno-xcore-fptrgroup
    ${CMAKE_CURRENT_LIST_DIR}/src/config.xscope
)

set(APP_COMPILE_DEFINITIONS
    configENABLE_DEBUG_PRINTF=1
    PLATFORM_USES_TILE_0=1
    PLATFORM_USES_TILE_1=1
    QSPI_FLASH_FILESYSTEM_START_ADDRESS=${FILESYSTEM_START_ADDRESS}
    QSPI_FLASH_MODEL_START_ADDRESS=${MODEL_START_ADDRESS}
    QSPI_FLASH_CALIBRATION_ADDRESS=${CALIBRATION_PATTERN_START_ADDRESS}
    ASR_CYBERON=1
)

set(APP_LINK_OPTIONS
    -report
    -lotp3
    ${CMAKE_CURRENT_LIST_DIR}/src/config.xscope
)

set(APP_COMMON_LINK_LIBRARIES
    sln_voice::app::ffd::ap
    sln_voice::app::asr::Cyberon
    sln_voice::app::asr::device_memory
    sln_voice::app::asr::gpio_ctrl
    sln_voice::app::asr::intent_engine
    sln_voice::app::asr::intent_handler
    sln_voice::app::ffd::xk_voice_l71
    lib_src
    lib_sw_pll
)

#**********************
# Tile Targets
#**********************
set(TARGET_NAME tile0_example_ffd_cyberon)
add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL)
target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES} ${RTOS_CONF_INCLUDES})
target_compile_definitions(${TARGET_NAME} PUBLIC ${APP_COMPILE_DEFINITIONS} THIS_XCORE_TILE=0)
target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
target_link_libraries(${TARGET_NAME} PUBLIC ${APP_COMMON_LINK_LIBRARIES})
target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
unset(TARGET_NAME)

set(TARGET_NAME tile1_example_ffd_cyberon)
add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL)
target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES} ${RTOS_CONF_INCLUDES})
target_compile_definitions(${TARGET_NAME} PUBLIC ${APP_COMPILE_DEFINITIONS} THIS_XCORE_TILE=1)
target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
target_link_libraries(${TARGET_NAME} PUBLIC ${APP_COMMON_LINK_LIBRARIES})
target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS} )
unset(TARGET_NAME)

#**********************
# Merge binaries
#**********************
merge_binaries(example_ffd_cyberon tile0_example_ffd_cyberon tile1_example_ffd_cyberon 1)

#**********************
# Create run and debug targets
#**********************
create_run_target(example_ffd_cyberon)
create_debug_target(example_ffd_cyberon)

#**********************
# Create data partition support targets
#**********************
set(TARGET_NAME example_ffd_cyberon)
set(DATA_PARTITION_FILE ${TARGET_NAME}_data_partition.bin)
set(MODEL_FILE ${TARGET_NAME}_model.bin)
set(FATFS_FILE ${TARGET_NAME}_fat.fs)
set(FLASH_CAL_FILE ${LIB_QSPI_FAST_READ_ROOT_PATH}/lib_qspi_fast_read/calibration_pattern_nibble_swap.bin)

add_custom_target(${MODEL_FILE} ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${CYBERON_COMMAND_NET_FILE} ${MODEL_FILE}
    COMMENT
        "Copy Cyberon NET file"
    VERBATIM
)

create_filesystem_target(
    #[[ Target ]]                   ${TARGET_NAME}
    #[[ Input Directory ]]          ${CMAKE_CURRENT_LIST_DIR}/filesystem_support/${MODEL_LANGUAGE}
    #[[ Image Size ]]               ${FILESYSTEM_SIZE_BYTES}
)

add_custom_command(
    OUTPUT ${DATA_PARTITION_FILE}
    COMMAND ${CMAKE_COMMAND} -E rm -f ${DATA_PARTITION_FILE}
    COMMAND datapartition_mkimage -v -b 1
    -i ${FLASH_CAL_FILE}:${CALIBRATION_PATTERN_DATA_PARTITION_OFFSET} ${FATFS_FILE}:${FILESYSTEM_DATA_PARTITION_OFFSET} ${MODEL_FILE}:${MODEL_DATA_PARTITION_OFFSET}
    -o ${DATA_PARTITION_FILE}
    DEPENDS
        ${MODEL_FILE}
        make_fs_${TARGET_NAME}
        ${FLASH_CAL_FILE}
    COMMENT
        "Create data partition"
    VERBATIM
)

set(DATA_PARTITION_FILE_LIST
    ${DATA_PARTITION_FILE}
    ${MODEL_FILE}
    ${FATFS_FILE}
    ${FLASH_CAL_FILE}
)

set(DATA_PARTITION_DEPENDS_LIST
    ${DATA_PARTITION_FILE}
    ${MODEL_FILE}
    make_fs_${TARGET_NAME}
)

# The list of files to copy and the dependency list for populating
# the data partition folder are identical.
create_data_partition_directory(
    #[[ Target ]]                   ${TARGET_NAME}
    #[[ Copy Files ]]               "${DATA_PARTITION_FILE_LIST}"
    #[[ Dependencies ]]             "${DATA_PARTITION_DEPENDS_LIST}"
)

create_flash_app_target(
    #[[ Target ]]                   ${TARGET_NAME}
    #[[ Boot Partition Size ]]      ${BOOT_PARTITION_SIZE}
    #[[ Data Partition Contents ]]  ${DATA_PARTITION_FILE}
    #[[ Dependencies ]]             ${DATA_PARTITION_FILE}
)

unset(DATA_PARTITION_FILE_LIST)
unset(DATA_PARTITION_DEPENDS_LIST)
