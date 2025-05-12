USRP B210
=================

The USRP B210 is a much higher end SDR.
It is much less user friendly than other SDRs.
This tutorial will aim to streamline the information provided on the
`USRP Manual <https://files.ettus.com/manual/page_build_guide.html>`_.
The tutorial is intended for a Linux system running Ubuntu, adjusting for
another OS would require viewing the Manual, however some steps are identical.

Installing Dependencies
----------------------------------

USRP devices require a set of dependencies, listed on their website.
They provide a command that should install all required dependencies.

.. code-block:: console

    sudo apt-get install autoconf automake build-essential ccache cmake cpufrequtils doxygen ethtool \
    g++ git inetutils-tools libboost-all-dev libncurses5 libncurses5-dev libusb-1.0-0 libusb-1.0-0-dev \
    libusb-dev python3-dev python3-mako python3-numpy python3-requests python3-scipy python3-setuptools \
    python3-ruamel.yaml


.. note::

    If on Ubuntu 24.04 then libncurses5 is no longer available.
    Running the following commands will install the packages correctly.

    .. code-block:: console

        wget http://archive.ubuntu.com/ubuntu/pool/universe/n/ncurses/libtinfo5_6.3-2_amd64.deb && sudo dpkg -i libtinfo5_6.3-2_amd64.deb && rm -f libtinfo5_6.3-2_amd64.deb

        wget http://archive.ubuntu.com/ubuntu/pool/universe/n/ncurses/libncurses5_6.3-2_amd64.deb && sudo dpkg -i libncurses5_6.3-2_amd64.deb && rm -f libncurses5_6.3-2_amd64.deb

        sudo apt install libncurses5 libncurses5-dev -y

With these commands the required dependencies should be installed.

Install Source Code
----------------------------------

Navigate to a new directory and clone the repository using:

.. code-block:: console

    git clone https://github.com/EttusResearch/uhd.git

By default it will be on the master branch which is the most up to date
version of the software. If a specific version is desired switching branches
will accomplish this (don't forget to navigate to the repo directory):

.. code-block:: console

    git checkout UHD-version


To build the file the Makefiles will need to be generated. Run the following:

.. code-block:: console

    cd <uhd-repo-path>/host
    mkdir build
    cd build
    cmake ../

.. note::

    The `USRP Manual <https://files.ettus.com/manual/page_build_guide.html>`_
    gives additional direction into common bugs and install configurations.

Once the build files are generated they can be built
using the following commands (this will take some time):

.. code-block:: console

    make
    make test # This step is optional
    sudo make install


