#!/bin/bash
#   ============================================================================
#   @file   make_release_package.sh
#
#   @desc   Script for EthFW release package
#
#   ============================================================================
#   Revision History
#   09-une-2023  Sonu          Initial version for Ethfw packaging script
#
#   ============================================================================
# Recommended options for calling the script: ./make_release_package.sh --platform="j721e" --release_version="09_00_00_00"

#Get user input
while [ $# -gt 0 ]; do
  case "$1" in
    --platform=*)
      platform="${1#*=}"
      ;;
    --release_version=*)
      release_version="${1#*=}"
      ;;
      -h|--help)
      echo Usage: $0 [options]
      echo
      echo Options,
      echo --release_version          Release version of format 09_00_00_00. This is must.
      echo --platform                 Options: j721e, j7200, j784s4
      exit 0
      ;;
  esac
  shift
done

#Default value if not provided
if [ "$working_dir" == "" ]; then
  working_dir=`pwd`
fi
: ${platform:="j721e"}
: ${release_version:="mm_nn_pp_bb"}

release_version_short=`echo ${release_version} | cut -d "_" -f -3`

product_name=ethfw
release_folder=${product_name}_${soc}_${release_version_short}

#Calculate num job to run in parallel - take one more to consider IO bound delays etc..
num_proc=`nproc`
num_jobs=`expr "${num_proc}" + "1"`
jobs_option="-j${num_jobs}"
echo "Working Directory: ${working_dir}"

ethfw_dir=$working_dir/../../
ethfw_rel_dir=$ethfw_dir/../$release_folder
log_dir=$ethfw_dir/ethfw_logs

####################### Make Ethfw release package ######################
make_rel_package() {
    echo "  Package EthFw for $release_version_short release..."

    mkdir -p $ethfw_rel_dir
    mkdir -p $log_dir
    
    cd ${ethfw_dir}

    local soc_caps=${platform^^}
    echo "    soc_caps:${soc_caps} ..."
    echo "    Building EthFw SOC:${soc_caps} ..."
    make -j -s ethfw_all BUILD_SOC_LIST=${soc_caps} PROFILE=release 1>>$log_dir/ethfw_build.log  2>>$log_dir/ethfw_error.log
    echo "    Building EthFw SOC:${soc_caps} Done"


    #User/API Guide generation
    echo "    EthFw User and API Guide generation..."
    make -C internal_docs/doxygen -s all DOXYGEN=doxygen 1>>$log_dir/ethfw_build.log  2>>$log_dir/ethfw_error.log
    echo "    EthFw User and API Guides completed!!"
    echo "    EthFw Datasheet generation..."
    make -C internal_docs/doxygen -s datasheet DOXYGEN=doxygen 1>>$log_dir/ethfw_build.log  2>>$log_dir/ethfw_error.log
    cp -R internal_docs/datasheet/ docs/.
    echo "    EthFw Datasheet completed!!"

    #Copy files to build folder
    cp -rf ${ethfw_dir} ${ethfw_rel_dir}

    #Change component paths
    sed -i -e "s|\/pdk|\/pdk_${soc}_${release_version_short}|g"   ${ethfw_rel_dir}/ethfw_tools_path.mak

    #Remove internal folders and components
    rm -rf ${ethfw_rel_dir}/.git
    rm -rf ${ethfw_rel_dir}/.gitignore
    rm -rf ${ethfw_rel_dir}/internal_docs
    cd - > /dev/null

    #Make Tar package
    cd ${ethfw_rel_dir}
    tar czf ${ethfw_rel_dir}.tar.gz ./* 1>>$log_dir/ethfw_build.log  2>>$log_dir/ethfw_error.log
    
    rm -rf ${ethfw_rel_dir}

    echo "  Package EthFw ... Done"
}

make_rel_package

exit

