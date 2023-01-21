############################
Check Device Firmware Update
############################

This test is a verification of the FFVA device firmware update (DFU) feature.  

**************************
Building and Running Tests
**************************

.. note::

    The Python environment is required to run this test.  See the Requirements section of test/README.rst
    This tests dfu-util commands.  Installation instructions for respective operating system can be found `here <https://dfu-util.sourceforge.net/>`__

To build the test application firmware and filesystem files, run the following command from the top of the repository: 

.. code-block:: console

    bash tools/ci/build_tests.sh

The `build_test.sh` script will copy the test applications and filesystem files to the `dist` folder.

Generate dfu-util image files with the following command from the top of the repository:

.. code-block:: console

    bash test/device_firmware_update/check_dfu.sh <firmware> <path-to-output-dir>

All paths must be absolute.  Relative paths may cause errors.

The actions performed by dfu-util can be verified by running a pytest for hash equality:

.. code-block:: console

    pytest test/device_firmware_update/test_dfu.py --readback_image <path-to-output-dir>/readback_upgrade.bin --upgrade_image <path-to-output-dir>/example_ffva_ua_adec_test_upgrade.bin