#/bin/sh
#
# Script to rebase repos having custom branch for Ethernet Subsystem Development
#

declare -A custom_branch_repos
custom_branch_repos["processor-sdk/common-csl-ip"]="origin/j7_master"
custom_branch_repos["processor-sdk/transport"]="origin/master"
custom_branch_repos["processor-sdk/emac-lld"]="origin/master"

for key in ${!custom_branch_repos[@]}; 
do
	if [[ ("$REPO_PROJECT" = ${key}) ]]
	then
		#REPO_PROJECT is set to the unique name of the project.
		echo "REPO_PROJECT $REPO_PROJECT"

		#REPO_PATH is the path relative to the root of the client.
		echo "REPO_PATH $REPO_PATH"

		#REPO_REMOTE is the name of the remote system from the manifest.
		echo "REPO_REMOTE $REPO_REMOTE"

		#REPO_LREV is the name of the revision from the manifest, translated to a local tracking branch. Used if you need to pass the manifest revision to a locally executed git command.
		echo "REPO_LREV $REPO_LREV"

		#REPO_RREV is the name of the revision from the manifest, exactly as written in the manifest.
		echo "REPO_RREV $REPO_RREV"
        
		#Do rebase from main branch to custom branch
    	echo `pwd`
		echo ${key} ${custom_branch_repos[${key}]}
		git rebase ${custom_branch_repos[${key}]}
	fi
done



