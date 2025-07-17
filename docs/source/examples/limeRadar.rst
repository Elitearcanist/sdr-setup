Lime Mini Radar
=====================
An interesting application for the Lime Mini is using it as a radar.
The Lime Mini has enough timing capabilities and precision to be a radar,
but enough problems to not be a great radar.
Still it is interesting and a good learning opportunity.

.. warning::
    The Lime Mini does not make a good radar. It can detect large radar targets
    (such as a corner reflector or cars) at short distances.

The radar will need soapySDR with the Limesuite module to function.
See the tutorial on installing :doc:`/tools/soapySDR`
and the Limesuite module: :doc:`/sdr/limemini`.

Using the Code
----------------------
The code for running the radar is found in the
repository in the examples folder under limeRadar.
For convenience it is also posted below.
The runner.sh script will automatically run the radar
code and the data plotter. It should be setup and verified beforehand.

Below is the shell script.

.. code:: shell

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


-L $LIB_DIR
    is where the soapySDR library is installed. The :code:`-L` command
    passes a specific location to search in. It is configured through the
    :code:`LIB_DIR` variable. It may not be necessary to include.
EXE_FILE
    is the file the compiler outputs.
CODE_FILE
    the name of the radar file.
PLOTTING_FILE
    the name of the data plotter file.
