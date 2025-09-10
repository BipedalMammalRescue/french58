#!/bin/bash

temp_path=$(pwd)/build/bin:$(pwd)/bin:$(pwd)/scripts
echo "Entering code shell, $temp_path will be temporarily added to the PATH environment variable."
PATH=$temp_path:$PATH
echo "To exit code shell, run \"exit\""
bash
echo "Exiting code shell..."