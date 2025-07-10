# Written by Levi Powell, May 2023
# This script compiles, executes, and plots the results of limeRadar.c

CODE_FILE="limeRadar.c"
EXE_FILE="limeRadar.out"
PLOTTING_FILE="dataPlotter5.py"
LIB_DIR="/usr/local/lib"

RED='\033[0;31m'    # Red
YEL='\033[0;33m'    # Yellow
BLU='\033[0;34m'    # Blue
NC='\033[0m'        # No Color

echo $YEL[RUNNER] Compiling $CODE_FILE ...$NC
gcc -std=c99 $CODE_FILE -lSoapySDR -lm -Ofast -o $EXE_FILE -L $LIB_DIR

echo $YEL[RUNNER] Executing $EXE_FILE ...$NC
./$EXE_FILE

echo $YEL[RUNNER] Plotting with $PLOTTING_FILE ...$NC
python3 $PLOTTING_FILE
