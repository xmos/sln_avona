
.. _sln_voice_ffd_deploying_linux_mac:

******************************************
Deploying the Firmware with Linux or macOS
******************************************

This document explains how to deploy the software using *CMake* and *Make*.

.. note::

    In the commands below ``<speech_engine>`` can be either ``sensory`` or ``cyberon``, depending on the choice of the speech recognition engine and model.

.. note::

    The Cyberon speech recognition engine is integrated in two examples. The ``example_ffd_cyberon`` use the microphone array as the audio source, and the ``example_ffd_i2s_input_cyberon`` uses the |I2S| interface as the audio source.
    In the rest of this section, we use only the ``example_ffd_<speech_engine>`` as an example.

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

After having your python environment activated, run the following commands in the root folder to build the firmware:

.. code-block:: console

    pip install -r requirements.txt
    cmake -B build --toolchain=xmos_cmake_toolchain/xs3a.cmake
    cd build
    make example_ffd_<speech_engine>

Running the Firmware
====================

Before running the firmware, the filesystem and model must be flashed to the
data partition.

Within the root of the build folder, run:

.. code-block:: console

    make flash_app_example_ffd_<speech_engine>

After this command completes, the application will be running.

After flashing the data partition, the application can be run without
reflashing. If changes are made to the data partition components, the
application must be reflashed.

From the build folder run:

.. code-block:: console

    xrun --xscope example_ffd_<speech_engine>.xe

Debugging the Firmware
======================

To debug with xgdb, from the build folder run:


.. code-block:: console

    xgdb -ex "connect --xscope" -ex "run" example_ffd_<speech_engine>.xe
