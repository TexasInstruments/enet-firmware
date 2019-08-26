#!/bin/bash
#see the end of file for standard work flow
prefix=$(date +%Y-%m-%d-%H%M)
tarfolder=${prefix}_review
#remove this folder if it exist
rm $tarfolder -rf

gitstfile="$tarfolder/gitst.txt"
modi="modified"
orig="orig"

if [ $# -eq 2 ];then
    modcommitid="$2"
else
    modcommitid="HEAD"
fi

mkdir $tarfolder
git diff --stat=256,256 $1 $modcommitid | grep --invert-match Bin | sed -n 's/|.*$//p' > $gitstfile

##########################################################
#function to put file $1(maybe include path) into folder $2
############################################################
put1to2 () {
    if [ $# -eq 2 ];then
        dir=$tarfolder/$2/$(dirname $1)
        if [ -f $1 ];then
		  mkdir -p $dir
          cp $1 $dir
		fi
    else
        echo "wrong number of parameter for call put1to2,called with $#, but need 2"
        exit 1
    fi


}



#do real work to get original file and modified file
exec 3<> $gitstfile
while read line <&3
do {
    statline=$line
    #check if there was a rename
	if [[ $statline == *"=>"* ]]; then
	  if [[ $statline == *"{"* ]]; then
	    common_prefix=`echo ${statline} | sed -e 's/^\(.*\){\{1\}.*$/\1/g'`
	    common_suffix=`echo ${statline} | sed -e 's/^.*}\{1\}\(.*\)$/\1/g'`
        origfilepath="${common_prefix}`echo ${statline} | sed -e 's/^.*{\{1\}\(.*\) => .*$/\1/g'`${common_suffix}"
	    modfilepath="${common_prefix}`echo ${statline} | sed -e 's/^.* => \(.*\)}\{1\}.*$/\1/g'`${common_suffix}"
      else
        origfilepath=`echo ${statline} | sed -e 's/^\(.*\) => .*$/\1/g'`
	    modfilepath=`echo ${statline} | sed -e 's/^.* => \(.*\)$/\1/g'`
      fi
	else
	  origfilepath=$statline
	  modfilepath=$statline
    fi	
	#here we get the original file and put it into original folder
    dir=$tarfolder/$orig/$(dirname $origfilepath)
    mkdir -p $dir
    origfile=$tarfolder/$orig/$origfilepath
    git rev-parse --verify $1:$origfilepath >/dev/null 2>&1 &&  git cat-file blob $1:$origfilepath > $origfile
    if [ "$modcommitid" = "HEAD" ];then 
        #here we handle the modified file, copy them to modified folder
        put1to2 $modfilepath $modi
    else
        modifiedfile=$tarfolder/$modi/$modfilepath
        dir=$(dirname $modifiedfile)
        mkdir -p $dir
        git rev-parse --verify $2:$modfilepath >/dev/null 2>&1 &&  git cat-file blob $2:$modfilepath > $modifiedfile
    fi
	
}
done
exec 3>&-

cat $gitstfile
echo "will create review tar ball $tarfolder"
tar zcf ${tarfolder}.tgz $tarfolder


