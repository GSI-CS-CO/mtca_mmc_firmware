#!/bin/bash

echo "Gathering info to generate build info header..."


build_date=$(date)

git_user=$(git config user.name)


if [[ $git_user ]]; then
  echo "$git_user"
else
  echo "Git user name not set -- please run: git config --global user.name your-name-here"
  exit 1
fi

git_email=$(git config user.email)
if [[ $git_email ]]; then
  echo "$git_email"
else  echo  "Git user email not set -- please run: git config --global user.email your-email-here"
  exit 1
fi

git_branch=$(git rev-parse --abbrev-ref HEAD)
if [[ $git_branch ]]; then
  echo "$git_branch"
else
  echo "Failed to determine current git branch"
  exit 1
fi


git_count=$(git rev-list --count HEAD)

if [[ $git_count ]]; then
  echo "$git_count"
else
  echo "Failed to determine commit count"
  exit 1
fi

git_source_info="$git_branch-$git_count"
os_info=$(cut -d' ' -f2- <<< $(lsb_release -d))
git_log=$(git log --oneline --decorate=no -n 5)
gcc_version=$(arm-none-eabi-gcc --version | grep eabi)

#write build info to header file
echo "const char *MMC_BUILD_ID =\"\\"            > ../src/build_id.h
echo "-- Build info -----------------------\n\\">> ../src/build_id.h
echo "Project     : FTRN AMC MMC\n\\"           >> ../src/build_id.h
echo "Build date  : $build_date\n\\"            >> ../src/build_id.h
echo "Prepared by : $git_user <$git_email>\n\\" >> ../src/build_id.h
echo "Source info : $git_source_info\n\\"       >> ../src/build_id.h
echo "OS version  : $os_info\n\\"               >> ../src/build_id.h
echo "Compiler    : $gcc_version\n\\"           >> ../src/build_id.h
echo "\n\\"                                     >> ../src/build_id.h

while IFS= read -r log_line
do
  echo "$log_line\n\\" >> ../src/build_id.h
done <<< "$git_log"


echo "\n\n\";" >> ../src/build_id.h

