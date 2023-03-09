# Automated Speech Recognition Porting

This is the XCORE-VOICE automated speech recognition (ASR) porting example application.

## Supported Hardware

This example is supported on the XCORE-AI-EXPLORER board.  However, the XK_VOICE_L71 board can be supported with a couple minor modifications.

## Building the host server

This application requires a host application to serve files to the device. The served file must be named `test.wav`.  This filename is defined in `src/app_conf.h`.

Run the following commands in the root folder to build the host application using your native x86 Toolchain:

**Permissions may be required to install the host applications.**

### Linux or Mac

    cmake -B build_host
    cd build_host
    make xscope_host_endpoint
    make install

The host application, `xscope_host_endpoint`, will be installed at `/opt/xmos/bin/`, and may be moved if desired.  You may wish to add this directory to your `PATH` variable.

Before running the host application, you may need to add the location of `xscope_endpoint.so` to your `LD_LIBRARY_PATH` environment variable.  This environment variable will be set if you run the host application in the XTC Tools command-line environment.  For more information see `Configuring the command-line environment <https://www.xmos.ai/documentation/XM-014363-PC-LATEST/html/tools-guide/install-configure/getting-started.html>`__.

### Windows

Before building the host application, you will need to add the path to the XTC Tools to your environment.

    set "XMOS_TOOL_PATH=<path-to-xtc-tools>"

Then build the host application:

    cmake -G "NMake Makefiles" -B build_host
    cd build_host
    nmake xscope_host_endpoint
    nmake install

The host application, `xscope_host_endpoint.exe`, will install at `<USERPROFILE>\.xmos\bin`, and may be moved if desired.  You may wish to add this directory to your `PATH` variable.

Before running the host application, you may need to add the location of `xscope_endpoint.dll` to your `PATH`. This environment variable will be set if you run the host application in the XTC Tools command-line environment.  For more information see `Configuring the command-line environment <https://www.xmos.ai/documentation/XM-014363-PC-LATEST/html/tools-guide/install-configure/getting-started.html>`__.

## Building the Firmware

Run the following commands in the root folder to build the firmware:

On Linux and Mac run:

    cmake -B build -DCMAKE_TOOLCHAIN_FILE=xmos_cmake_toolchain/xs3a.cmake
    cd build
    make example_asr

On Windows run:

    cmake -G "NMake Makefiles" -B build -D CMAKE_TOOLCHAIN_FILE=xmos_cmake_toolchain/xs3a.cmake
    cd build
    nmake example_asr

## Flashing the Model

Run the following commands in the build folder to flash the model:

    xflash --force --quad-spi-clock 50MHz --factory example_asr.xe --boot-partition-size 0x100000 --target-file ../examples/speech_recognition/XCORE-AI-EXPLORER.xn --data ../examples/speech_recognition/asr/port/example/asr_example_model.dat

## Running the Firmware

Run the following command in the build folder to run the firmware.

On Linux and Mac run:

    make run_example_asr

On Windows run:

    nmake run_example_asr

In a second console, run the following command in the ``examples/speech_recognition`` folder to run the host server:

On Linux and Mac run:

    xscope_host_endpoint 12345

On Windows run:

    xscope_host_endpoint.exe 12345
