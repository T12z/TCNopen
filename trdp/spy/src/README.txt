                                                               
                  _____ _____ ____  _____    _____ _____ __ __ 
                 |_   _| __  |    \|  _  |  |   __|  _  |  |  |
                   | | |    -|  |  |   __|  |__   |   __|_   _|
                   |_| |__|__|____/|__|     |_____|__|    |_|  
                                                               

															   
This Plugin can be used to display packages containing TRDP (Train Realtime Data Protocol).

The Plugin was developed for wireshark version 2.2.7.

S E T U P
=========

Download the source code from wireshark for the project's homepage [1].
Follow the build instructions for the used operating system [2].

*Copy* the *trdp_spy* folder into the "plugins" subfolder of your wireshark source directory.
*Copy CMakeListsCustom.txt* into the root of the wirshark sources, (next to the plugins folder).



[1] http://www.wireshark.org/download.html
[2] http://www.wireshark.org/docs/wsdg_html_chunked/ChapterSetup.html


B U I L D I N G
===============

Generate a new folder, as described in the build instructions, next to the source code. E.g. wireshark-custom

Windows
-------

Copy (and probalby adapt) the two deliverd bat files from the subfolder scripts into the new generated wireshark-custom folder.
Open there a command prompt and execute the following commands:
> setupEnv.bat
> cmake ../wireshark-2.2.7/
> runIDE.bat

Now Visual Studio is opened, where the complete solution can be rebuilt.


Linux
-----

In the folder wireshark-custom, the following command will generate wireshark with the plugin:
$ cd wireshark-custom/
$ cmake ../wireshark-2.2.7/
$ make

Hint: The following packages where installed on ubuntu: 
$ sudo apt install qttools5-dev qttools5-dev-tools libqt5svg5-dev qtmultimedia5-dev cmake libkrb5-dev libkrb5-26-heimdal byacc flex libpcap-dev
