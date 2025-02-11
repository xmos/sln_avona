
#**********************
# Gather Sources
#**********************
file(GLOB_RECURSE APP_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.c )
set(APP_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/wav
)

include(${CMAKE_CURRENT_LIST_DIR}/bsp_config/bsp_config.cmake)

#**********************
# QSPI Flash Layout
#**********************
set(BOOT_PARTITION_SIZE 0x100000)

set(CALIBRATION_PATTERN_START_ADDRESS ${BOOT_PARTITION_SIZE})
set(CALIBRATION_PATTERN_DATA_PARTITION_OFFSET 0)

math(EXPR MODEL_START_ADDRESS
    "${CALIBRATION_PATTERN_START_ADDRESS} + ${LIB_QSPI_FAST_READ_DEFAULT_CAL_SIZE_BYTES}"
    OUTPUT_FORMAT HEXADECIMAL
)

math(EXPR MODEL_DATA_PARTITION_OFFSET
    "${CALIBRATION_PATTERN_DATA_PARTITION_OFFSET} + ${LIB_QSPI_FAST_READ_DEFAULT_CAL_SIZE_BYTES}"
    OUTPUT_FORMAT DECIMAL
)


#**********************
# Setup ASR test to build
#**********************
if(NOT DEFINED TEST_ASR)
    message(STATUS "No test ASR specified")
    return()
endif()

if(${TEST_ASR} STREQUAL "SENSORY")
    message(STATUS "Building Sensory ASR test")
    set(ASR_LIBRARY sln_voice::app::asr::sensory)
    set(ASR_BRICK_SIZE_SAMPLES 240)
    set(APP_SOURCES
        ${APP_SOURCES}
        ${FFD_SRC_ROOT}/model/english_usa/command-pc62w-6.4.0-op10-prod-search.c
    )
    set(MODEL_FILE ${FFD_SRC_ROOT}/model/english_usa/command-pc62w-6.4.0-op10-prod-net.bin.nibble_swapped)
    set(TEST_ASR_LIBRARY_ID 0)
    set(TEST_ASR_NAME test_asr_sensory)
    set(ASR_FLAG ASR_SENSORY=1)

elseif(${TEST_ASR} STREQUAL "CYBERON")
    message(STATUS "Building Cyberon ASR test")
    set(ASR_LIBRARY sln_voice::app::asr::Cyberon)
    set(ASR_BRICK_SIZE_SAMPLES 240)
    set(APP_SOURCES
        ${APP_SOURCES}
    )
    #set(MODEL_FILE ${FFD_SRC_ROOT}/model/english_usa/Hello_XMOS_pack_WithTxt.bin.Enc.NibbleSwap)
    set(MODEL_FILE ${FFD_SRC_ROOT}/model/english_usa/Testing_level3_pack_WithTxt.bin.Enc.NibbleSwap)
    set(TEST_ASR_LIBRARY_ID 1)
    set(TEST_ASR_NAME test_asr_cyberon)
    set(ASR_FLAG ASR_CYBERON=1)
else()
    message(FATAL_ERROR "Unable to build ${TEST_ASR} test")
endif()

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
    DEBUG_PRINT_ENABLE=1
    PLATFORM_USES_TILE_0=1
    PLATFORM_USES_TILE_1=1
    XUD_CORE_CLOCK=600
    XSCOPE_HOST_IO_ENABLED=1
    XSCOPE_HOST_IO_TILE=0
    ${ASR_FLAG}
    QSPI_FLASH_CALIBRATION_ADDRESS=${CALIBRATION_PATTERN_START_ADDRESS}
    QSPI_FLASH_MODEL_START_ADDRESS=${MODEL_START_ADDRESS}
    appconfASR_LIBRARY_ID=${TEST_ASR_LIBRARY_ID}
    appconfASR_BRICK_SIZE_SAMPLES=${ASR_BRICK_SIZE_SAMPLES}
)

set(APP_LINK_OPTIONS
    -report
    ${CMAKE_CURRENT_LIST_DIR}/src/config.xscope
    -lotp3
)

set(APP_COMMON_LINK_LIBRARIES
    rtos::freertos
    xscope_fileio
    sln_voice_test_asr_board_support_xk_voice_l71
)

#**********************
# Tile Targets
#**********************
set(TARGET_NAME tile0_${TEST_ASR_NAME})
add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL)
target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
target_compile_definitions(${TARGET_NAME}
    PUBLIC
        ${APP_COMPILE_DEFINITIONS}
        THIS_XCORE_TILE=0
)
target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
target_link_libraries(${TARGET_NAME}
    PUBLIC
        ${APP_COMMON_LINK_LIBRARIES}
        ${ASR_LIBRARY}
        sln_voice::app::asr::device_memory
)
target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
unset(TARGET_NAME)

set(TARGET_NAME tile1_${TEST_ASR_NAME})
add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL)
target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
target_compile_definitions(${TARGET_NAME}
    PUBLIC
        ${APP_COMPILE_DEFINITIONS}
        THIS_XCORE_TILE=1
)
target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
target_link_libraries(${TARGET_NAME}
    PUBLIC
        ${APP_COMMON_LINK_LIBRARIES}
        ${ASR_LIBRARY}
        sln_voice::app::asr::device_memory
)
target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
unset(TARGET_NAME)

#**********************
# Merge binaries
#**********************
merge_binaries(${TEST_ASR_NAME} tile0_${TEST_ASR_NAME} tile1_${TEST_ASR_NAME} 1)

#**********************
# Create data partition target
#**********************
set(DATA_PARTITION_FILE ${TEST_ASR_NAME}_data_partition.bin)
set(FLASH_CAL_FILE ${LIB_QSPI_FAST_READ_ROOT_PATH}/lib_qspi_fast_read/calibration_pattern_nibble_swap.bin)

add_custom_command(
    OUTPUT ${DATA_PARTITION_FILE}
    COMMAND ${CMAKE_COMMAND} -E rm -f ${DATA_PARTITION_FILE}
    COMMAND datapartition_mkimage -v -b 1
    -i ${FLASH_CAL_FILE}:${CALIBRATION_PATTERN_DATA_PARTITION_OFFSET} ${MODEL_FILE}:${MODEL_DATA_PARTITION_OFFSET}
    -o ${DATA_PARTITION_FILE}
    DEPENDS
        ${MODEL_FILE}
        ${FLASH_CAL_FILE}
    COMMENT
        "Create data partition"
    VERBATIM
)

set(DATA_PARTITION_FILE_LIST
    ${DATA_PARTITION_FILE}
    ${FLASH_CAL_FILE}
)

set(DATA_PARTITION_DEPENDS_LIST
    ${DATA_PARTITION_FILE}
)

# The list of files to copy and the dependency list for populating
# the data partition folder are identical.
create_data_partition_directory(
    #[[ Target ]]                   ${TEST_ASR_NAME}
    #[[ Copy Files ]]               "${DATA_PARTITION_FILE_LIST}"
    #[[ Dependencies ]]             "${DATA_PARTITION_DEPENDS_LIST}"
)

create_flash_app_target(
    #[[ Target ]]                   ${TEST_ASR_NAME}
    #[[ Boot Partition Size ]]      ${BOOT_PARTITION_SIZE}
    #[[ Data Partition Contents ]]  ${DATA_PARTITION_FILE}
    #[[ Dependencies ]]             ${DATA_PARTITION_FILE}
)

#**********************
# Create run target
#**********************
add_custom_target(run_${TEST_ASR_NAME}
  COMMAND xrun --xscope --xscope-port localhost:12345 ${TEST_ASR_NAME}.xe
  DEPENDS ${TEST_ASR_NAME}
  COMMENT
    "Run application"
  VERBATIM
)
