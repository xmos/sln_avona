#**********************
# ASR Language info
#**********************

set(MODEL_LANGUAGE "english_usa")
#set(CYBERON_COMMAND_NET_FILE "${CMAKE_CURRENT_LIST_DIR}/asr/model/english_usa/Hello_XMOS_pack_WithTxt.bin.Enc.NibbleSwap")
set(CYBERON_COMMAND_NET_FILE "${CMAKE_CURRENT_LIST_DIR}/asr/model/english_usa/Cyberon_XMOS_Command_pack_WithTxt.bin.Enc.NibbleSwap")

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

set(FFVA_INT_CYBERON_COMPILE_DEFINITIONS
    ${APP_COMPILE_DEFINITIONS}
    appconfEXTERNAL_MCLK=1
    appconfI2S_ENABLED=1
    appconfUSB_ENABLED=0
    appconfINTENT_ENABLED=1
    appconfAEC_REF_DEFAULT=appconfAEC_REF_I2S
    appconfI2S_MODE=appconfI2S_MODE_SLAVE
    appconfI2S_AUDIO_SAMPLE_RATE=48000
    configENABLE_DEBUG_PRINTF=1
    appconfRECOVER_MCLK_I2S_APP_PLL=1
    QSPI_FLASH_FILESYSTEM_START_ADDRESS=${FILESYSTEM_START_ADDRESS}
    QSPI_FLASH_MODEL_START_ADDRESS=${MODEL_START_ADDRESS}
    QSPI_FLASH_CALIBRATION_ADDRESS=${CALIBRATION_PATTERN_START_ADDRESS}
    ASR_CYBERON=1
    MIC_ARRAY_CONFIG_MCLK_FREQ=12288000
)

foreach(FFVA_AP ${FFVA_PIPELINES_INT})
    #**********************
    # Tile Targets
    #**********************
    set(TARGET_NAME tile0_example_ffva_int_cyberon_${FFVA_AP})
    add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL)
    target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
    target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
    target_compile_definitions(${TARGET_NAME}
        PUBLIC
            ${FFVA_INT_CYBERON_COMPILE_DEFINITIONS}
            THIS_XCORE_TILE=0
    )
    target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
    target_link_libraries(${TARGET_NAME}
        PUBLIC
            ${APP_COMMON_LINK_LIBRARIES}
            sln_voice::app::ffva::xk_voice_l71
            sln_voice::app::ffva::ap::${FFVA_AP}
            sln_voice::app::asr::Cyberon
            sln_voice::app::asr::device_memory
            sln_voice::app::asr::gpio_ctrl
            sln_voice::app::asr::intent_engine
            sln_voice::app::asr::intent_handler
    )
    target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
    unset(TARGET_NAME)

    set(TARGET_NAME tile1_example_ffva_int_cyberon_${FFVA_AP})
    add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL)
    target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
    target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
    target_compile_definitions(${TARGET_NAME}
        PUBLIC
            ${FFVA_INT_CYBERON_COMPILE_DEFINITIONS}
            THIS_XCORE_TILE=1
    )
    target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
    target_link_libraries(${TARGET_NAME}
        PUBLIC
            ${APP_COMMON_LINK_LIBRARIES}
            sln_voice::app::ffva::xk_voice_l71
            sln_voice::app::ffva::ap::${FFVA_AP}
            sln_voice::app::asr::Cyberon
            sln_voice::app::asr::device_memory
            sln_voice::app::asr::gpio_ctrl
            sln_voice::app::asr::intent_engine
            sln_voice::app::asr::intent_handler
    )
    target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
    unset(TARGET_NAME)

    #**********************
    # Merge binaries
    #**********************
    merge_binaries(example_ffva_int_cyberon_${FFVA_AP} tile0_example_ffva_int_cyberon_${FFVA_AP} tile1_example_ffva_int_cyberon_${FFVA_AP} 1)

    #**********************
    # Create run and debug targets
    #**********************
    create_run_target(example_ffva_int_cyberon_${FFVA_AP})
    create_debug_target(example_ffva_int_cyberon_${FFVA_AP})

    #**********************
    # Create data partition support targets
    #**********************
    set(TARGET_NAME example_ffva_int_cyberon_${FFVA_AP})
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

endforeach()
