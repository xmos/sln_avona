
.. _sln_voice_ffva_deploying_linux_macos_programming_guide:

******************************************
Deploying the Firmware with Linux or macOS
******************************************

This document explains how to deploy the software using *CMake* and *Make*.

Building the Host Applications
==============================

This application requires a host application to create the flash data partition. Run the following commands in the root folder to build the host application using your native Toolchain:

.. note::

  Permissions may be required to install the host applications.

.. code-block:: console

  cmake -B build_host
  cd build_host
  make install

The host applications will be installed at ``/opt/xmos/bin``, and may be moved if desired.  You may wish to add this directory to your ``PATH`` variable.

Building the Firmware
=====================

After having your python environment activated, run the following commands in the root folder to build the |I2S| firmware:

.. code-block:: console

    pip install -r requirements.txt
    cmake -B build --toolchain=xmos_cmake_toolchain/xs3a.cmake
    cd build
    make example_ffva_int_fixed_delay

After having your python environment activated, run the following commands in the root folder to build the |I2S| firmware with the Cyberon ASR engine:

.. code-block:: console

    pip install -r requirements.txt
    cmake -B build --toolchain=xmos_cmake_toolchain/xs3a.cmake
    cd build
    make example_ffva_int_cyberon_fixed_delay

After having your python environment activated, run the following commands in the root folder to build the USB firmware:

.. code-block:: console

    pip install -r requirements.txt
    cmake -B build --toolchain=xmos_cmake_toolchain/xs3a.cmake
    cd build
    make example_ffva_ua_adec_altarch

Running the Firmware
====================

Before the firmware is run, the filesystem must be loaded.

Inside of the build folder root, after building the firmware, run one of:

.. code-block:: console

    make flash_app_example_ffva_int_fixed_delay
    make flash_app_example_ffva_int_cyberon_fixed_delay
    make flash_app_example_ffva_ua_adec_altarch

Once flashed, the application will run.

After the filesystem has been flashed once, the application can be run without flashing.  If changes are made to the filesystem image, the application must be reflashed.

From the build folder run:

.. code-block:: console

    xrun --xscope example_ffva_int_fixed_delay.xe
    xrun --xscope example_ffva_int_cyberon_fixed_delay.xe
    xrun --xscope example_ffva_ua_adec_altarch.xe

Upgrading the Firmware
======================

UA variant
----------

The UA variants of this application contain DFU over the USB DFU Class V1.1 transport method.

To create an upgrade image from the build folder run:

.. code-block:: console

    make create_upgrade_img_example_ffva_ua_adec_altarch

Once the application is running, a USB DFU v1.1 tool can be used to perform various actions.  This example will demonstrate with dfu-util commands.  Installation instructions for the respective operating systems can be found `here <https://dfu-util.sourceforge.net/>`__.

To verify the device is running run:

.. code-block:: console

    dfu-util -l

This should result in an output containing:

.. code-block:: console

    Found DFU: [20b1:4001] ver=0001, devnum=100, cfg=1, intf=3, path="3-4.3", alt=2, name="DFU DATAPARTITION", serial="123456"
    Found DFU: [20b1:4001] ver=0001, devnum=100, cfg=1, intf=3, path="3-4.3", alt=1, name="DFU UPGRADE", serial="123456"
    Found DFU: [20b1:4001] ver=0001, devnum=100, cfg=1, intf=3, path="3-4.3", alt=0, name="DFU FACTORY", serial="123456"

The DFU interprets the flash as 3 separate partitions, the read only factory image, the read/write upgrade image, and the read/write data partition containing the filesystem.

The factory image can be read back by running:

.. code-block:: console

    dfu-util -e -d ,20b1:4001 -a 0 -U readback_factory_img.bin

The factory image can not be written to.

From the build folder, the upgrade image can be written by running:

.. code-block:: console

    dfu-util -e -d ,20b1:4001 -a 1 -D example_ffva_ua_adec_altarch_upgrade.bin

The upgrade image can be read back by running:

.. code-block:: console

    dfu-util -e -d ,20b1:4001 -a 1 -U readback_upgrade_img.bin

On system reboot, the upgrade image will always be loaded if valid.  If the upgrade image is invalid, the factory image will be loaded.  To revert back to the factory image, you can upload a file containing the word 0xFFFFFFFF.

The data partition image can be read back by running:

.. code-block:: console

    dfu-util -e -d ,20b1:4001 -a 2 -U readback_data_partition_img.bin

The data partition image can be written by running:

.. code-block:: console

    dfu-util -e -d ,20b1:4001 -a 2 -D readback_data_partition_img.bin

Note that the data partition will always be at the address specified in the initial flashing call.

INT variant
-----------

The INT variants of this application contain DFU over |I2C|.

To create an upgrade image from the build folder run:

.. code-block:: console

    make create_upgrade_img_example_ffva_int_fixed_delay

Once the application is running, the *xvf_dfu* tool can be used to perform various actions. Installation instructions for Raspbian OS can be found `here <https://github.com/xmos/host_xvf_control/blob/main/README.rst>`__.

Before running the *xvf_dfu* host application, the ``I2C_ADDRESS`` value in the file ``transport_config.yaml`` located in the same folder as the binary file ``xvf_dfu`` must be updated. This value must match the one set for ``appconf_CONTROL_I2C_DEVICE_ADDR`` in the ``platform_conf.h`` file.

The DFU interprets the flash as 3 separate partitions, the read only factory image, the read/write upgrade image, and the read/write data partition containing the filesystem.

The factory image can be read back by running:

.. code-block:: console

    xvf_dfu --upload-factory readback_factory_img.bin

The factory image can not be written to.

From the build folder, the upgrade image can be written by running:

.. code-block:: console

    xvf_dfu -d example_ffva_int_fixed_delay_upgrade.bin

The upgrade image can be read back by running:

.. code-block:: console

    xvf_dfu --upload-upgrade readback_upgrade_img.bin

The device can be rebooted remotely by running

.. code-block:: console

    xvf_dfu --reboot

On system reboot, the upgrade image will always be loaded if valid.  If the upgrade image is invalid, the factory image will be loaded.  To revert back to the factory image, you can upload a file containing the word 0xFFFFFFFF.

The FFVA-INT variants include some version numbers:

  - *APP_VERSION_MAJOR*
  - *APP_VERSION_MINOR*
  - *APP_VERSION_PATCH*

These values are defined in the ``app_conf.h`` file, and they can read by running:

.. code-block:: console

    xvf_dfu --version

The data partition image cannot be read or write using the *xvf_dfu* host application.

Debugging the Firmware
======================

To debug with xgdb, from the build folder run:

.. code-block:: console

    xgdb -ex "connect --xscope" -ex "run" example_ffva_int_fixed_delay.xe
    xgdb -ex "connect --xscope" -ex "run" example_ffva_ua_adec_altarch.xe
