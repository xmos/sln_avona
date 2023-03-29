# XCORE:registered:-VOICE Solution Repository

The XCORE-VOICE Solution consists of example designs and a C-based SDK for the development of audio front-end applications to support far-field voice use cases on the xcore.ai family of chips (XU316). The XCORE-VOICE design is currently based on FreeRTOS, leveraging the flexibility of the xcore.ai platform and providing designers with a familiar environment to customize and develop products.

XCORE-VOICE example designs provide turn-key solutions to enable easier product development for smart home applications such as light switches, thermostats, and home appliances. xcore.ai’s unique architecture providing powerful signal processing and accelerated AI capabilities combined with the XCORE-VOICE framework allows designers to incorporate keyword, event detection, or advanced local dictionary support to create a complete voice interface solution. 

## Build Status

Build Type       |    Status     |
-----------      | --------------|
CI (Linux)       | ![CI](https://github.com/xmos/sln_voice/actions/workflows/ci.yml/badge.svg?branch=develop&event=push) |
Docs             | ![CI](https://github.com/xmos/sln_voice/actions/workflows/docs.yml/badge.svg?branch=develop&event=push) |

## Cloning

Some dependent components are included as git submodules. These can be obtained by cloning this repository with the following command:

    $ git clone --recurse-submodules git@github.com:xmos/sln_voice.git

## Documentation

See the READMEs for the early example applications:

[FFD](https://github.com/xmos/sln_voice/blob/develop/examples/ffd/README.md)

[FFVA](https://github.com/xmos/sln_voice/blob/develop/examples/ffva/README.md)

# Getting Help

A [Github issue](https://github.com/xmos/sln_voice/issues/new/choose) should be the primary method of getting in touch with the XMOS development team.

## License

This Software is subject to the terms of the [XMOS Public Licence: Version 1](https://github.com/xmos/sln_voice/blob/develop/LICENSE.rst)

Additional third party copyrighted code is included under the following licenses:

The Wanson speech recognition library is `Copyright 2022. Shanghai Wanson Electronic Technology Co.Ltd (&quot;WANSON&quot;)` and is subject to the [Wanson Restrictive License](https://github.com/xmos/sln_voice/blob/develop/examples/ffd/asr/port/wanson/lib/LICENSE.md).

