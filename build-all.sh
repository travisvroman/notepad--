#!/bin/bash
# Build script for cleaning and/or building everything
PLATFORM="$1"
ACTION="$2"
TARGET="$3"

set echo off

txtgrn=$(echo -e '\e[0;32m')
txtred=$(echo -e '\e[0;31m')
txtrst=$(echo -e '\e[0m')

if [ $ACTION = "all" ] || [ $ACTION = "build" ]
then
   ACTION="all"
   ACTION_STR="Building"
   ACTION_STR_PAST="built"
elif [ $ACTION = "clean" ]
then
   ACTION="clean"
   ACTION_STR="Cleaning"
   ACTION_STR_PAST="cleaned"
else
   echo "Unknown action $ACTION. Aborting" && exit
fi

echo "$ACTION_STR everything on $PLATFORM ($TARGET)..."


# Testbed
make -f Makefile.executable.mak $ACTION TARGET=$TARGET ASSEMBLY=notepadmm ADDL_INC_FLAGS="-I$VULKAN_SDK/include" ADDL_LINK_FLAGS="-lvulkan -lshaderc_shared -L$VULKAN_SDK/lib"

ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL | sed -e "s/Error/${txtred}Error${txtrst}/g" && exit
fi

echo "All assemblies $ACTION_STR_PAST successfully on $PLATFORM ($TARGET)." | sed -e "s/successfully/${txtgrn}successfully${txtrst}/g"

