#!/bin/bash
# This script is used to fetch and install dependencies for the package build
#

# Grab and install XDC
function install_xdc()
{
    local version=${1}

    echo "Install: XDCtools ${version}"

    if [ -d "xdctools_${version}" ]; then
        echo "XDCTools already installed."
    else
        version_numonly=$(echo ${version} | sed 's/\_core//g' | sed 's/\_eng//g')
        if [ ! -f xdctools_${version}_linux.zip ]; then
            wget --no-proxy -nc http://www.sanb.design.ti.com/tisb_releases/XDCtools/${version_numonly}/exports/xdccore/xdctools_${version}_linux.zip
        else
            echo "xdctools_${version}_linux.zip already exists, not re-downloading."
        fi
        unzip -q xdctools_${version}_linux.zip
    fi
}

# Grab and install SYS/BIOS
function install_bios()
{
    local version=${1}

    echo "Install: BIOS ${version}"

    if [ -d "bios_${version}" ]; then
        echo "SYS/BIOS already installed."
    else
        if [ ! -f bios_${version}.run ]; then
            wget --no-proxy -nc http://www.sanb.design.ti.com/tisb_releases/BIOS/${version}/exports/bios_${version}.run
        else
            echo "bios_${version}.run already exists, not re-downloading."
        fi
        chmod a+x bios_${version}.run
        ./bios_${version}.run --prefix ./ --mode unattended
    fi
}

# Grab and install SYS/BIOS (K3 engineering release)
function install_bios_k3()
{
    local version=${1}

    echo "Install: K3 BIOS ${version}"

    if [ -d "bios_${version}" ]; then
        echo "SYS/BIOS already installed."
    else
        if [ ! -f bios_${version}.zip ]; then
            wget --no-proxy -nc http://bangsdowebsvr01.india.ti.com/PROCESSOR_SDK_RTOS_AUTOMOTIVE/swdownloads/bios_${version}.zip
        else
            echo "bios_${version}.zip already exists, not re-downloading."
        fi
        unzip -q bios_${version}.zip
    fi
}

# Grab and unpack NDK
function install_ndk()
{
    local version=${1}

    echo "Install: NDK ${version}"

    if [ -d "ndk_${version}" ]; then
        echo "NDK already installed."
    else
        if [ ! -f ndk_${version}.zip ]; then
            wget --no-proxy -nc http://www.sanb.design.ti.com/tisb_releases/NDK/${version}/exports/ndk_${version}.zip
        else
            echo "ndk_${version}.zip already exists, not re-downloading."
        fi
        unzip -q ndk_${version}.zip
    fi
}

# Grab and unpack NS
function install_ns()
{
    local version=${1}

    echo "Install: NS ${version}"

    if [ -d "ns_${version}" ]; then
        echo "NS already installed."
    else
        if [ ! -f ns_${version}.zip ]; then
            wget --no-proxy -nc http://www.sanb.design.ti.com/tisb_releases/NS/${version}/exports/ns_${version}.zip
        else
            echo "ns_${version}.zip already exists, not re-downloading."
        fi
        unzip -q ns_${version}.zip
    fi
}

# Grab and install TI ARM CGT
function install_ti_arm_cgt()
{
    local version=${1}

    echo "Install: TI ARM CGT ${version}"

    if [ -d "ti-cgt-arm_${version}" ]; then
        echo "TI CGT Compiler already installed"
    else
        wget http://syntaxerror.dal.design.ti.com/release/releases/arm/rel${version//./_}/build/install/ti_cgt_tms470_${version}_linux_installer_x86.bin --no-check-certificate
        chmod +x ti_cgt_tms470_${version}_linux_installer_x86.bin
        ./ti_cgt_tms470_${version}_linux_installer_x86.bin  --mode unattended --prefix ./
    fi
}

# Grab and unpack Linaro GCC compiler
function install_linaro_gcc_linux()
{
    local version=${1}
    local version_web=$(echo ${version} | sed -r 's:(.{3})..(.*):\1\2:g')

    echo "Install: Linaro Linux GCC ${version}"

    if [ -d gcc-linaro-${version}-x86_64_aarch64-linux-gnu ]; then
        echo "Linaro Linux GCC already installed"
    else
        wget https://releases.linaro.org/components/toolchain/binaries/${version_web}/aarch64-linux-gnu/gcc-linaro-${version}-x86_64_aarch64-linux-gnu.tar.xz --no-check-certificate
        chmod +x gcc-linaro-${version}-x86_64_aarch64-linux-gnu.tar.xz
        tar xf gcc-linaro-${version}-x86_64_aarch64-linux-gnu.tar.xz
    fi
}

# Grab and unpack Linaro GCC compiler
function install_linaro_gcc_baremetal()
{
    local version=${1}
    local version_web=$(echo ${version} | sed -r 's:(.{3})..(.*):\1\2:g')

    echo "Install: Linaro Baremetal GCC ${version}"

    if [ -d gcc-linaro-${version}-x86_64_aarch64-elf ]; then
        echo "Linaro baremetal GCC already installed"
    else
        wget https://releases.linaro.org/components/toolchain/binaries/${version_web}/aarch64-elf/gcc-linaro-${version}-x86_64_aarch64-elf.tar.xz --no-check-certificate
        chmod +x gcc-linaro-${version}-x86_64_aarch64-elf.tar.xz
        tar xf gcc-linaro-${version}-x86_64_aarch64-elf.tar.xz
    fi
}

# Klocwork Tools
function install_klocwork()
{
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
}

# Webgen Tool
# Todo

source ./release_info.sh

cd ../../../
echo "Current directory: `pwd`"

install_ndk ${OLD_NDK}
install_ns ${OLD_NS}
install_bios ${OLD_BIOS_AM65XX}
install_bios ${OLD_BIOS_J721E}
install_xdc ${OLD_XDC}
install_linaro_gcc_linux ${OLD_GCC_ARCH64}
install_linaro_gcc_baremetal ${OLD_GCC_ARCH64}
install_ti_arm_cgt ${OLD_CGT_ARM_AM65XX}
install_ti_arm_cgt ${OLD_CGT_ARM_J721E}

cd -

# OS packages
sudo apt-get -y install doxygen
echo Get Dependencies done!!

