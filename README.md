## About

**espHLG** is a firmware for the ESP8266/8285 designed to run on the Helium Lite Gateway board.

## Compile

1. Download version 2.2.x of the non-OS SDK from https://github.com/espressif/ESP8266_NONOS_SDK/archive/release/v2.2.x.tar.gz and extract it to your working folder:

        cd /your/downloads
        wget https://github.com/espressif/ESP8266_NONOS_SDK/archive/release/v2.2.x.tar.gz
        tar xvf v2.2.x.tar.gz -C ${WORK_FOLDER}

2. Get the toolchain from https://qtoggle.s3.amazonaws.com/various/xtensa-lx106-elf.tar.xz and extract it to your working folder:

        cd /your/downloads
        wget https://qtoggle.s3.amazonaws.com/various/xtensa-lx106-elf.tar.xz
        tar xvf xtensa-lx106-elf.tar.xz -C ${WORK_FOLDER}

3. Prepare the environment:

        export XTENSA_TOOLS_ROOT=${WORK_FOLDER}/xtensa-lx106-elf
        export SDK_BASE=${WORK_FOLDER}/ESP8266_NONOS_SDK-release-v2.2.x
        export PATH=${XTENSA_TOOLS_ROOT}/bin:$PATH

4. Clone this repo into your working folder:

        cd ${WORK_FOLDER}
        git clone git@github.com:ccrisan/esphlg.git

5. Apply required fixes/patches to the SDK:

        ${WORK_FOLDER}/esphlg/builder/fix-sdk.sh ${SDK_BASE}

6. Build the firmware using `make`:

        cd ${WORK_FOLDER}/esphlg
        make

   Your firmware should be waiting for you at `${WORK_FOLDER}/esphlg/build/firmware.bin`. Just write it at address `0` on your ESP device.
