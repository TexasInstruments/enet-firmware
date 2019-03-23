#!/bin/bash
# This script is used to fetch and install dependencies for the package build
#

cd ..
make env.sh
. ./env.sh
cd -

cd ${TOOLS_DIR}

# Grab and install XDC
if [ -d "xdctools_${XDC_VERSION}" ]; then
	echo "XDCTools already installed."
else
    XDC_VERSION_NUMONLY=$(echo ${XDC_VERSION} | sed 's/\_core//g')
    if [ ! -f xdctools_${XDC_VERSION}_linux.zip ]; then
        wget --no-proxy -nc http://www.sanb.design.ti.com/tisb_releases/XDCtools/${XDC_VERSION_NUMONLY}/exports/xdccore/xdctools_${XDC_VERSION}_linux.zip
    else
        echo "xdctools_${XDC_VERSION}_linux.zip already exists, not re-downloading."
    fi
    unzip -q xdctools_${XDC_VERSION}_linux.zip
fi

# Grab and install SYS/BIOS
if [ -d "bios_${BIOS_VERSION}" ]
then
	echo "SYS/BIOS already installed."
else
    if [ ! -f bios_setuplinux_${BIOS_VERSION}.bin ]; then
        wget --no-proxy -nc http://www.sanb.design.ti.com/tisb_releases/BIOS/${BIOS_VERSION}/exports/bios_setuplinux_${BIOS_VERSION}.bin
    else
        echo "bios_setuplinux_${BIOS_VERSION}.bin already exists, not re-downloading."
    fi
    chmod a+x bios_setuplinux_${BIOS_VERSION}.bin
	./bios_setuplinux_${BIOS_VERSION}.bin --prefix ${TOOLS_DIR} --mode unattended
fi

# Grab and unpack NDK
if [ -d "ndk_${NDK_VERSION}" ]; then
	echo "NDK already installed."
else
    if [ ! -f ndk_${NDK_VERSION}.zip ]; then
        wget --no-proxy -nc http://www.sanb.design.ti.com/tisb_releases/NDK/${NDK_VERSION}/exports/ndk_${NDK_VERSION}.zip
    else
        echo "ndk_${NDK_VERSION}.zip already exists, not re-downloading."
    fi
    unzip -q ndk_${NDK_VERSION}.zip
fi

# Grab and unpack UIA
if [ -d "uia_${UIA_VERSION}" ]; then
	echo "UIA already installed."
else
    if [ ! -f uia_${UIA_VERSION}.zip ]; then
        wget --no-proxy -nc http://www.sanb.design.ti.com/tisb_releases/UIA/${UIA_VERSION}/exports/uia_${UIA_VERSION}.zip
    else
        echo "uia_${UIA_VERSION}.zip already exists, not re-downloading."
    fi
	unzip -q uia_${UIA_VERSION}.zip
fi

# Grab and unpack EDMA3
if [ -d "edma3_lld_${EDMA3_VERSION}" ]; then
	echo "EDMA3_LLD already installed."
else
    if [ ! -f edma3_lld_${EDMA3_VERSION}.tar.gz ]; then
        wget --no-proxy -nc http://downloads.ti.com/dsps/dsps_public_sw/sdo_tii/psp/edma3_lld/edma3-lld-bios6/${EDMA3_VERSION}/exports/edma3_lld_${EDMA3_VERSION}.tar.gz
    else
        echo "edma3_lld_${EDMA3_VERSION}.tar.gz already exists, not re-downloading."
    fi
	tar xzf edma3_lld_${EDMA3_VERSION}.tar.gz
fi
# Grab and install TMS470 ARM compiler
if [ ! -d "${COMPILER_DIR}" ]; then
    mkdir -p ${COMPILER_DIR}
fi

if [ -d "${TIARMCGT_ROOT}" ]; then
	echo TMS470 ARM Compiler already installed
else
    TIARMCGT_VERSION=$(basename ${TIARMCGT_ROOT} | cut -d '_' -f 2)
    TIARMCGT_VERSION_US=$(echo ${TIARMCGT_VERSION} | sed 's/\./\_/g')

    if [ ! -f ti_cgt_tms470_${TIARMCGT_VERSION}_linux_installer_x86.bin ]; then
    	wget --no-proxy -nc http://syntaxerror.dal.design.ti.com/release/releases/arm/rel${TIARMCGT_VERSION_US}/build/install/ti_cgt_tms470_${TIARMCGT_VERSION}_linux_installer_x86.bin
    else
        echo "ti_cgt_tms470_${TIARMCGT_VERSION}_linux_installer_x86.bin already exists, not re-downloading."
    fi
    chmod a+x ti_cgt_tms470_${TIARMCGT_VERSION}_linux_installer_x86.bin
    sudo ./ti_cgt_tms470_${TIARMCGT_VERSION}_linux_installer_x86.bin --prefix $(dirname ${TIARMCGT_ROOT}) --mode unattended
    sudo chmod 777 -R ${TIARMCGT_ROOT}
fi

# Grab and install the C6x DSP compiler
if [ -d "$CGT6X_ROOT" ]
then
	echo TMS320C6000 Compiler already installed
else
    CGT6X_VERSION=$(basename ${CGT6X_ROOT} | cut -d '_' -f 2)
    CGT6X_VERSION_US=$(echo ${CGT6X_VERSION} | sed 's/\./\_/g')

    if [ ! -f ti_cgt_c6000_${CGT6X_VERSION}_setup_linux_x86.bin ]; then
        wget --no-proxy -nc http://syntaxerror.dal.design.ti.com/release/releases/c60/rel${CGT6X_VERSION_US}/build/install/ti_cgt_c6000_${CGT6X_VERSION}_setup_linux_x86.bin
    else
        echo "ti_cgt_c6000_${CGT6X_VERSION}_linux_installer_x86.bin already exists, not re-downloading."
    fi
    chmod a+x ti_cgt_c6000_${CGT6X_VERSION}_setup_linux_x86.bin
    echo sudo ./ti_cgt_c6000_${CGT6X_VERSION}_setup_linux_x86.bin --prefix $(dirname ${CGT6X_ROOT})/C6000_${CGT6X_VERSION} --mode silent
    sudo ./ti_cgt_c6000_${CGT6X_VERSION}_setup_linux_x86.bin --prefix $(dirname ${CGT6X_ROOT})/C6000_${CGT6X_VERSION} --mode silent
    sudo chmod 777 -R ${CGT6X_ROOT}
fi

# Grab and unpack Linaro GCC compiler
if [ -d "${GCC_ROOT}" ]
then
	echo Linaro GCC already installed
else
    if [ ! -f /gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 ]; then
        wget -nc https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
    else
        echo "gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 already exists, not re-downloading."
    fi
	sudo tar xjf gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 -C ${COMPILER_DIR}
    sudo chmod 777 -R ${GCC_ROOT}
fi

# Klocwork Tools
if [ -d "/opt/klocWork" ]; then
    echo Klocwork Insight tool already installed.
else
    if [ ! -f kw-insight-cmd-installer.linux64.sh ]; then
        wget --no-proxy -nc https://klocwork.dal.design.ti.com/portal/downloads/kw-insight-cmd-installer.linux64.sh
    else
        echo "kw-insight-cmd-installer.linux64.sh already exists, not re-downloading."
    fi
    chmod +x kw-insight-cmd-installer.linux64.sh
    mkdir /opt/klocwork
    chmod 777 /opt/klocwork
    chown $USER /opt/klocwork
    ./kw-insight-cmd-installer.linux64.sh --agree --install-dir /opt/klocwork --force --klocwork-server klocwork.dal.design.ti.com:8090 --license-server flames-usa4.sc.ti.com:27005 --use-ssl

    pushd /opt/klocwork/plugins > /dev/null
    wget --no-proxy -N http://sdit.dal.design.ti.com/Klocwork/kw10.1.2/misra/misra_c.xml
    wget --no-proxy -N http://sdit.dal.design.ti.com/Klocwork/kw10.1.2/misra/misra_cpp.xml
    popd
    pushd /opt/klocwork/config > /dev/null
    wget --no-proxy -N http://sdit.dal.design.ti.com/Klocwork/kw_filters/kw_filters_10.x/cl6x_filter.py
    wget --no-proxy -N http://sdit.dal.design.ti.com/Klocwork/kw_filters/kw_filters_10.x/cl430_filter.py
    wget --no-proxy -N http://sdit.dal.design.ti.com/Klocwork/kw_filters/kw_filters_10.x/kwfilter.conf
    popd
fi

cd -

# Webgen Tool
# Todo

# OS packages
sudo apt-get -y install doxygen
echo Get Dependencies done!!

