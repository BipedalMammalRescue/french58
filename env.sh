#!/bin/bash

project_root=$(dirname "$0")
cd $project_root
project_root=$(pwd)

temp_path=$project_root/build/bin:$project_root/bin:$project_root/Scripts
echo "Entering code shell, $temp_path will be temporarily added to the PATH environment variable."
PATH=$temp_path:$PATH
echo "To exit code shell, run \"exit\""

asset_root=$project_root/Assets project_root=$project_root bash

echo "Exiting code shell..."