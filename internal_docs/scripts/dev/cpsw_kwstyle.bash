#!/bin/bash

function Log() {
    echo "$1" &>> ${Logfile}
}

function Headline() {
    Log ""
    Log "======================================================"
    Log " File: $1 "
    Log "======================================================"

    echo "Checking file: $1"
}

function checkAllFiles() {
    files=`find . -name "*.[ch]"`

    for f in $files; do
        Headline "${f}"
        KWStyle -v -xml ${ConfigFile} ${f} &>> ${Logfile}
    done
}

function checkAllFilesInMaster() {
    files=`git ls-tree -r origin/master --name-only | grep -e ".[ch]$"`
    for f in $files; do
        Headline "${f}"
        KWStyle -v -xml ${ConfigFile} ${f} &>> ${Logfile}
    done
}

function checkChangedFiles() {
    files=`git diff --stat origin/master HEAD --name-only`
    for f in $files; do
        Headline "${f}"
        KWStyle -v -xml ${ConfigFile} ${f} &>> ${Logfile}
    done
}

Me=`basename $0`
MyHome=`dirname $0`
ConfigFile="${MyHome}/cpsw.kws.xml"

kws=`which KWStyle`
if [ "${kws}" == "" ]; then
    echo "KWStyle not found, try: sudo apt-get install KWStyle"
    echo ""
    exit 1
fi

if ! [ -f ./cpsw_component.mk ]; then
    echo "${Me} should be called from CPSW LLD's base directory (i.e. <pdk>/packages/ti/drv/cpsw)"
    echo ""
    exit 1
fi

if ! [ -f ${ConfigFile} ]; then
    echo "XML config file not found, expected at: ${ConfigFile}"
    echo ""
    exit 1
fi

# Create log file
Logfile=`tempfile`

echo "XML config file: ${ConfigFile}"
echo ""

# Check all files in the CPSW LLD directory
#checkAllFiles

# Check all files under version control in 'master' branch
#checkAllFilesInMaster

# Check all files changed in current branch wrt 'origin/master'
checkChangedFiles

echo ""
echo "KWStyle logs: ${Logfile}"
echo ""
