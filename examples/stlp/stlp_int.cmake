
set(STLP_INT_COMPILE_DEFINITIONS
    ${APP_COMPILE_DEFINITIONS}
    appconfEXTERNAL_MCLK=1
    appconfI2S_ENABLED=1
    appconfUSB_ENABLED=0
    appconfAEC_REF_DEFAULT=appconfAEC_REF_I2S
    appconfI2S_MODE=appconfI2S_MODE_SLAVE
    appconfI2S_AUDIO_SAMPLE_RATE=48000
    
    MIC_ARRAY_CONFIG_MCLK_FREQ=12288000
)

foreach(STLP_AP ${STLP_PIPELINES})
    #**********************
    # Tile Targets
    #**********************
    set(TARGET_NAME tile0_example_stlp_int_${STLP_AP})
    add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL)
    target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
    target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
    target_compile_definitions(${TARGET_NAME}
        PUBLIC
            ${STLP_INT_COMPILE_DEFINITIONS}
            THIS_XCORE_TILE=0
    )
    target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
    target_link_libraries(${TARGET_NAME}
        PUBLIC
            ${APP_COMMON_LINK_LIBRARIES}
            sln_voice::app::stlp::xk_voice_l71
            sln_voice::app::stlp::ap::${STLP_AP}
    )
    target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
    unset(TARGET_NAME)

    set(TARGET_NAME tile1_example_stlp_int_${STLP_AP})
    add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL)
    target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
    target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
    target_compile_definitions(${TARGET_NAME}
        PUBLIC
            ${STLP_INT_COMPILE_DEFINITIONS}
            THIS_XCORE_TILE=1
    )
    target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
    target_link_libraries(${TARGET_NAME}
        PUBLIC
            ${APP_COMMON_LINK_LIBRARIES}
            sln_voice::app::stlp::xk_voice_l71
            sln_voice::app::stlp::ap::${STLP_AP}
    )
    target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
    unset(TARGET_NAME)

    #**********************
    # Merge binaries
    #**********************
    merge_binaries(example_stlp_int_${STLP_AP} tile0_example_stlp_int_${STLP_AP} tile1_example_stlp_int_${STLP_AP} 1)

    #**********************
    # Create run and debug targets
    #**********************
    create_run_target(example_stlp_int_${STLP_AP})
    create_debug_target(example_stlp_int_${STLP_AP})
    create_filesystem_target(example_stlp_int_${STLP_AP})
    create_flash_app_target(example_stlp_int_${STLP_AP})

    #**********************
    # Filesystem support targets
    #**********************

    if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL Windows)
        add_custom_command(
            OUTPUT example_stlp_int_${STLP_AP}_fat.fs
            COMMAND ${CMAKE_COMMAND} -E make_directory %temp%/fatmktmp/fs
            COMMAND ${CMAKE_COMMAND} -E copy demo.txt %temp%/fatmktmp/fs/demo.txt
            COMMAND fatfs_mkimage --input=%temp%/fatmktmp --output=example_stlp_int_${STLP_AP}_fat.fs
            BYPRODUCTS %temp%/fatmktmp
            DEPENDS example_stlp_int_${STLP_AP}
            COMMENT
                "Create filesystem"
            WORKING_DIRECTORY
                ${CMAKE_CURRENT_LIST_DIR}/filesystem_support
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT example_stlp_int_${STLP_AP}_fat.fs
            COMMAND bash -c "tmp_dir=$(mktemp -d) && fat_mnt_dir=$tmp_dir && mkdir -p $fat_mnt_dir && mkdir $fat_mnt_dir/fs && cp ./demo.txt $fat_mnt_dir/fs/demo.txt && fatfs_mkimage --input=$tmp_dir --output=example_stlp_int_${STLP_AP}_fat.fs"
            DEPENDS example_stlp_int_${STLP_AP}
            COMMENT
                "Create filesystem"
            WORKING_DIRECTORY
                ${CMAKE_CURRENT_LIST_DIR}/filesystem_support
            VERBATIM
        )
    endif()

    add_custom_target(flash_fs_example_stlp_int_${STLP_AP}
        COMMAND xflash --quad-spi-clock 50MHz --factory example_stlp_int_${STLP_AP}.xe --boot-partition-size 0x100000 --data ${CMAKE_CURRENT_LIST_DIR}/filesystem_support/example_stlp_int_${STLP_AP}_fat.fs
        DEPENDS example_stlp_fat_int_${STLP_AP}_fat.fs
        COMMENT
            "Flash filesystem"
        VERBATIM
    )
endforeach()
