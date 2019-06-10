#!/bin/bash
#   ============================================================================
#   @file   cm_accounting.sh
#
#   @desc   Script to get git diff summary between two tags
#
#   ============================================================================

#Get user input
while [ $# -gt 0 ]; do
  case "$1" in
    --cm_status_accounting)
      command="cm_status_accounting"
      ;;
    --old_tag=*)
      old_tag="${1#*=}"
      ;;
    --new_tag=*)
      new_tag="${1#*=}"
      ;;
    --working_dir=*)
      working_dir="${1#*=}"
      ;;
    *)
      printf "Error: Invalid argument $1!!\n"
  esac
  shift
done

if [ "$working_dir" == "" ]; then
  working_dir=`pwd`
  #Script called from within ethfw git for rebase/status
  cd ..
  ethfw_dir=`pwd`
else
  ethfw_dir=$working_dir/ethfw
  cd $ethfw_dir
fi

dir_list=(
	""
)

git_rebase() {
  git_dir=$ethfw_dir/$1
  if [ ! -d "$git_dir" ]; then
    return
  fi

  cd $git_dir
  echo === Rebase for $git_dir ...

  # stash changes if not clean
  #if git diff-index --quiet HEAD --; then
  if [ -n "$(git status --untracked-files=no --porcelain)" ]; then
    is_clean="no"
    git stash
  else
    is_clean="yes"
  fi

  # fetch and rebase
  git fetch; git rebase

  # apply if stashed
  if [ "$is_clean" == "no" ]; then
    git stash pop
  fi

  echo ""
}

run_cmd() {
  git_dir=$ethfw_dir/$1
  if [ ! -d "$git_dir" ]; then
    return
  fi

  cd $git_dir
  echo === $3 for $git_dir ...
  $2
  echo ""
}

git_diff() {
  run_cmd $1 "git diff" "Diff"
}

git_reset() {
  run_cmd $1 "git checkout ." "Reset"
}

git_branch() {
  git_dir=$ethfw_dir/$1
  if [ ! -d "$git_dir" ]; then
    return
  fi

  cd $git_dir
  printf "%-40s: " "ethfw/"$1
  git rev-parse --abbrev-ref HEAD
}

git_allclean() {
  run_cmd $1 "rm -rf lib" "Allclean"
}

git_status() {
  run_cmd $1 "git status" "Status"
}

git_prune() {
  run_cmd $1 "git fetch origin --prune" "Prune"
}

git_cleandxf() {
  run_cmd $1 "git clean -dxf" "git clean dxf"
}

git_onelinelog() {
  git_dir=$ethfw_dir/$1
  if [ ! -d "$git_dir" ]; then
    return
  fi

  cd $git_dir
  temp=`git log -n 1 --pretty=oneline`
  printf "%-40s: $temp\n" "ethfw/"$1
}

git_log() {
  run_cmd $1 "git log -n 1 --stat" "git log"
}

git_cm_status() {
  git_dir=$ethfw_dir/$1
  if [ ! -d "$git_dir" ]; then
    return
  fi
  cd $git_dir
  printf "=== CM Status Accounting for ethfw/$1 from release/tag $old_tag to $new_tag ...\n"
  printf "\n"

  printf "Number of Files Added   : "
  git log --pretty --oneline --name-status "$old_tag".."$new_tag" | sort -u | grep $'A\t' | cut -f 2 | wc -l
  printf "Number of Files Modified: "
  git log --pretty --oneline --name-status "$old_tag".."$new_tag" | sort -u | grep $'M\t' | cut -f 2 | wc -l
  printf "Number of Files Deleted : "
  git log --pretty --oneline --name-status "$old_tag".."$new_tag" | sort -u | grep $'D\t' | cut -f 2 | wc -l

  printf "\n"
  printf "List of Files Added:\n"
  git log --pretty --oneline --name-status "$old_tag".."$new_tag" | sort -u | grep $'A\t' | cut -f 2

  printf "\n"
  printf "List of Files Modified:\n"
  git log --pretty --oneline --name-status "$old_tag".."$new_tag" | sort -u | grep $'M\t' | cut -f 2

  printf "\n"
  printf "List of Files Deleted:\n"
  git log --pretty --oneline --name-status "$old_tag".."$new_tag" | sort -u | grep $'D\t' | cut -f 2

  echo ""
}

if [ "$command" == "cm_status_accounting" ]; then
	if [ x"" == x$old_tag ]; then
		printf "Need to provide old tag for CM status accounting!!\n"
		printf "Usage: \n    ./cm_accounting.sh --cm_status_accounting --old_tag=<\"REL.PDK.TDA.01.07.00.04\"> --new_tag=\"<. if not provided>\" \n"
		return
	fi
	for dir in "${dir_list[@]}"
	do
		git_cm_status "$dir"
	done
fi

cd $working_dir
return
