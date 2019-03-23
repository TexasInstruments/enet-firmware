#!/bin/bash
# This function is used to create a git tag in prep to make a release.
#
# Usage:
#      ./create_release.sh "<MAJOR VERSION NUMBER>_<MINOR VERSION NUMBER> [<NEW_PACKAGE_NAME>]"
#      ./create_release.sh
#

function cleanup()
{
    # Anything to clean up?
    echo "Cleaning up..."
}

function abort()
{
    cleanup
    exit 1
}

function update_project_dependency()
{
    local prefix_text=${1}

    # For some reason all numbers in the version lose the leading zero except the build number
    local major=$(echo ${2} | cut -d _ -f 1 | sed 's/0\([0-9]*\)/\1/g')
    local minor=$(echo ${2} | cut -d _ -f 2 | sed 's/0\([0-9]*\)/\1/g')
    local patch=$(echo ${2} | cut -d _ -f 3 | sed 's/0\([0-9]*\)/\1/g')
    local build=$(echo ${2} | cut -d _ -f 4)
    local old_dot_version=$(echo "${major}.${minor}.${patch}.${build}")

    if [ -z ${4} ]; then
        old_dot_version=$(echo "${major}.${minor}.${patch}.${build}")
    elif [ ${4} -eq 3 ]; then
        old_dot_version=$(echo "${major}.${minor}.${patch}")
    elif [ ${4} -eq 2 ]; then
        old_dot_version=$(echo "${major}.${minor}")
    fi

    local major=$(echo ${3} | cut -d _ -f 1 | sed 's/0\([0-9]*\)/\1/g')
    local minor=$(echo ${3} | cut -d _ -f 2 | sed 's/0\([0-9]*\)/\1/g')
    local patch=$(echo ${3} | cut -d _ -f 3 | sed 's/0\([0-9]*\)/\1/g')
    local build=$(echo ${3} | cut -d _ -f 4)
    local new_dot_version=$(echo "${major}.${minor}.${patch}.${build}")

    if [ -z ${4} ]; then
        new_dot_version=$(echo "${major}.${minor}.${patch}.${build}")
    elif [ ${4} -eq 3 ]; then
        new_dot_version=$(echo "${major}.${minor}.${patch}")
    elif [ ${4} -eq 2 ]; then
        new_dot_version=$(echo "${major}.${minor}")
    fi

    # The .cproject file
    for file in $(find ./packages/ti/ndk -type f -name .cproject); do
        echo "Editing file ${file}..."
        sed -i -b "s/${prefix_text}${old_dot_version}/${prefix_text}${new_dot_version}/g" ${file}
    done

    for file in $(find ./utils -type f -name .cproject); do
        echo "Editing file ${file}..."
        sed -i -b "s/${prefix_text}${old_dot_version}/${prefix_text}${new_dot_version}/g" ${file}
    done
}

function update_file()
{
    local searchdir=${1}
    local filename=${2}
    local prefix_text=${3}

    local old_underscore_version=${4}
    local new_underscore_version=${5}

    for file in $(find ${searchdir} -type f -name ${filename}); do
        echo "Editing file ${file}..."
        sed -i -b "s/${prefix_text}${old_underscore_version}/${prefix_text}${new_underscore_version}/g" ${file}
    done
}


function update_files()
{
    # Convert version tag from XX_YY_ZZ_WW to XX.YY.ZZ.WW, XX,YY,ZZ,WW
    local old_underscore_version=${1}
    local new_underscore_version=${2}
    local old_dot_version=$(echo ${1} | sed 's/\_/./g')
    local new_dot_version=$(echo ${2} | sed 's/\_/./g')
    local old_comma_version=$(echo ${1} | sed 's/\_/,/g')
    local new_comma_version=$(echo ${2} | sed 's/\_/,/g')

    local old_underscore_pkg_name=${3}
    local new_underscore_pkg_name=${4}
    local old_dot_pkg_name=$(echo ${3} | sed 's/\_/./g')
    local new_dot_pkg_name=$(echo ${4} | sed 's/\_/./g')

    local old_date="${5}"
    local new_date="${6}"

    # Save release_notes to relnotes_archive
    git checkout release/${old_underscore_version} ${old_underscore_pkg_name}_${old_underscore_version}_release_notes.html
    if [ $? -eq 0 ]; then
        cp ${old_underscore_pkg_name}_${old_underscore_version}_release_notes.html ./../relnotes_archive/${old_underscore_pkg_name}_${old_underscore_version}_release_notes.html
        git add ../relnotes_archive/${old_underscore_pkg_name}_${old_underscore_version}_release_notes.html
    fi
    git checkout HEAD ${old_underscore_pkg_name}_${old_underscore_version}_release_notes.html

    # The plugin files have the version and package name parameterized
    # so the actual values are filled in
    for file in $(find ./eclipse/ -type f); do
        echo "Editing file ${file}..."
        sed -i -b "s/${old_dot_version}/${new_dot_version}/g" ${file}
        sed -i -b "s/${old_dot_pkg_name}/${new_dot_pkg_name}/g" ${file}
        sed -i -b "s/${old_underscore_pkg_name}/${new_underscore_pkg_name}/g" ${file}
        sed -i -b "s/${old_date}/${new_date}/g" ${file}
    done

    # Update manifest, release notes, user's guide
    files="./${old_underscore_pkg_name}_${old_underscore_version}_manifest.html
           ./${old_underscore_pkg_name}_${old_underscore_version}_release_notes.html
           docs/${old_underscore_pkg_name}_${old_underscore_version}_user_guide.html
           ./webgen.mak
           ./scripts/release_info.sh"
    for file in ${files}; do
        echo "Editing file ${file}..."
        sed -i -b "s/${old_underscore_version}/${new_underscore_version}/g" ${file}
        sed -i -b "s/${old_underscore_pkg_name}/${new_underscore_pkg_name}/g" ${file}
        sed -i -b "s/${old_date}/${new_date}/g" ${file}
    done

    # Update the xdc package version number
    sed -i -b "s/${old_comma_version}/${new_comma_version}/g" ./packages/ti/nsp/drv/package.xdc

    # Do git mv on filenames that have changed
    git mv docs/${old_underscore_pkg_name}_${old_underscore_version}_user_guide.html docs/${new_underscore_pkg_name}_${new_underscore_version}_user_guide.html
    git mv ${old_underscore_pkg_name}_${old_underscore_version}_manifest.html      ${new_underscore_pkg_name}_${new_underscore_version}_manifest.html
    git mv ${old_underscore_pkg_name}_${old_underscore_version}_release_notes.html ${new_underscore_pkg_name}_${new_underscore_version}_release_notes.html

    # Use the actual version numbers in the plugin folders
    git mv eclipse/features/com.ti.rtsc.${old_dot_pkg_name}_${old_dot_version} eclipse/features/com.ti.rtsc.${new_dot_pkg_name}_${new_dot_version}
    git mv eclipse/plugins/com.ti.rtsc.${old_dot_pkg_name}.product_${old_dot_version} eclipse/plugins/com.ti.rtsc.${new_dot_pkg_name}.product_${new_dot_version}
}

# Make sure to remove temp directory when script is aborted
trap abort SIGINT

# Verify that we are running this script in the correct git working tree
if [ ! -e ./packages/ti/nsp/drv/package.xdc ]; then
    echo "Not running in the correct path! Aborting!"
    abort
fi
if [ ! -e ./.git ]; then
    echo "Not running in a valid GIT working tree."
    abort
fi

# Check git status to make sure working directory is clean (no outstanding changes needed)
STATUS=$(git status -s -uno)
if [ "${STATUS}" != "" ]; then
    echo "Your current working directory is not clean. Please commit all changes before tagging."
    echo "Git Status returned:"
    echo "${STATUS}"
    echo
    echo -n "Do you want to commit all changes right now (y/n)? "
    read COMMIT
    if [ "$COMMIT" == "y" ]; then
        echo -n "Enter commit message: "
        read COMMIT_MESSAGE
        git commit -a -m "${COMMIT_MESSAGE}"
    else
        echo "Please commit your changes manually and then rerun this script."
        abort
    fi
fi

# Read OLD (previous) release variables
. ./scripts/release_info.sh

echo "Previous build number:  ${OLD_VERSION}"
echo "Previous package name:  ${OLD_PKGNAME}"
echo "Previous package date:  ${OLD_PKGDATE}"
echo "Previous NDK version :  ${OLD_NDK}"
echo "Previous BIOS version:  ${OLD_BIOS}"
echo "Previous XDC version:   ${OLD_XDC}"
echo "Previous UIA version:   ${OLD_UIA}"
echo "Previous EDMA3 version: ${OLD_EDMA3}"

# Get current dependency version info
make env.sh
. ./env.sh

# Get the new package date
NEW_PKGDATE=$(date +"%m-%e-%Y")

# Read the new intended package name
if [ $# -lt 2 ]
then
    echo "Using the old package name (${OLD_PKGNAME}) as the new package name."
    NEW_PKGNAME="$OLD_PKGNAME"
else
    NEW_PKGNAME="$2"
fi

# Read/calculate the new intended version number so we can tag this release in the repository
if [ $# -lt 1 ]
then
    echo "Enter the release version in <x>_<yy> format (x=Major rev. no., yy=Minor rev. no.)"
    read NEW_VERSION
else
    NEW_VERSION="$1"
fi
NEW_VERSION_MAJOR=${NEW_VERSION%_*}
NEW_VERSION_MINOR=${NEW_VERSION#*_}

# Extract patch number and build number if a tag with major and minor already exist
OLD_VERSION_LIST=(${OLD_VERSION//_/ })
OLD_VERSION_MAJOR=${OLD_VERSION_LIST[0]}
OLD_VERSION_MINOR=${OLD_VERSION_LIST[1]}
if [ "$OLD_VERSION_MAJOR" == "$NEW_VERSION_MAJOR" ] && [ "$OLD_VERSION_MINOR" == "$NEW_VERSION_MINOR" ]; then
    PATCH=${OLD_VERSION_LIST[2]}; BUILD=${OLD_VERSION_LIST[3]}
    let PATCH+=0; let BUILD+=0
    # Increment patch number if fixes are in this release
    echo -n "Does this release contain bug fixes (y/n)? "
    read FIXES
    if [ "$FIXES" == "y" ]; then
        let PATCH+=1
    fi
    # Always increment build number
    let BUILD+=1
else
    # For new major_minor version
    PATCH=0
    BUILD=0
fi
NEW_VERSION_PATCH=$(printf "%02d" $PATCH)
NEW_VERSION_BUILD=$(printf "%02d" $BUILD)
NEW_VERSION=${NEW_VERSION_MAJOR}_${NEW_VERSION_MINOR}_${NEW_VERSION_PATCH}_${NEW_VERSION_BUILD}

echo ""
echo "New build number:  ${NEW_VERSION}"
echo "New package name:  ${NEW_PKGNAME}"
echo "New package date:  ${NEW_PKGDATE}"
echo "New NDK version :  ${NDK_VERSION}"
echo "New BIOS version:  ${BIOS_VERSION}"
echo "New XDC version:   ${XDC_VERSION}"
echo "New UIA version:   ${UIA_VERSION}"
echo "New EDMA3 version: ${EDMA3_VERSION}"

# Update files in the repo (manifests, release notes, etc).
update_files "${OLD_VERSION}" "${NEW_VERSION}" "${OLD_PKGNAME}" "${NEW_PKGNAME}" "${OLD_PKGDATE}" "${NEW_PKGDATE}"

# Update Dependency Versions in the example project files
update_project_dependency "com.ti.rtsc.NDK:" "${OLD_NDK}" "${NDK_VERSION}"
update_project_dependency "com.ti.rtsc.SYSBIOS:" "${OLD_BIOS}" "${BIOS_VERSION}"
update_project_dependency "XDC_VERSION=" "${OLD_XDC}" "${XDC_VERSION}"
update_project_dependency "com.ti.uia:" "${OLD_UIA}" "${UIA_VERSION}"

# Update package config.bld file
update_file ./packages config.bld "ndk_" "${OLD_NDK}" "${NDK_VERSION}"
update_file ./packages config.bld "bios_" "${OLD_BIOS}" "${BIOS_VERSION}"

# Update release_info.sh
update_file ./scripts release_info.sh "OLD_NDK=" "${OLD_NDK}" "${NDK_VERSION}"
update_file ./scripts release_info.sh "OLD_BIOS=" "${OLD_BIOS}" "${BIOS_VERSION}"
update_file ./scripts release_info.sh "OLD_XDC=" "${OLD_XDC}" "${XDC_VERSION}"
update_file ./scripts release_info.sh "OLD_UIA=" "${OLD_UIA}" "${UIA_VERSION}"

# Update the Static C and Misra C reports
NSP_ROOT=${PWD} ./scripts/kw_code_checks.sh

# Commit the updated package.xdc with the new version number
echo "Committing the release to current branch."
LOGMSG="${NEW_PKGNAME} Release, Ver. ${NEW_VERSION}, ${NEW_PKGDATE}"
git commit -a -m "${LOGMSG}"

# Tagging the release
echo "Tagging the release as release/${NEW_VERSION}."
git tag release/${NEW_VERSION}

# Push changes if desired
echo    "You have created a new release tag."
echo -n "Do you want to push these updates to the remote repository (y/n)? "
read PUSH_TO_REMOTE
if [ "$PUSH_TO_REMOTE" == "y" ]; then
    git push origin master
    git push --tags origin

    # Ping build server to kick off automated build
    echo "Kicking off Jenkins build of ${NEW_PKGNAME} Release, Ver. ${NEW_VERSION}."
    curl http://uda0271044.hou.asp.ti.com/jenkins/job/NSP_GMACSW_Release_Build/build?token=NSP_RELEASE_TOKEN
    echo "When done, release will be available at http://uda0271044.hou.asp.ti.com/targetcontent/${NEW_PKGNAME}/${NEW_VERSION}."
fi

echo "All operations complete."
cleanup
exit 0
