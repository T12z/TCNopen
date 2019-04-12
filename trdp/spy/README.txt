                                                               
                  _____ _____ ____  _____    _____ _____ __ __ 
                 |_   _| __  |    \|  _  |  |   __|  _  |  |  |
                   | | |    -|  |  |   __|  |__   |   __|_   _|
                   |_| |__|__|____/|__|     |_____|__|    |_|  
                                                               

															   
This Plugin can be used to display packages containing TRDP (Train Realtime Data Protocol).

The Plugin was previously developed for wireshark version 2.2.7.
However, the current version was tested with Wireshark 2.6 and 3.0. It may not work with earlier versions -- checkout older SVN revions if needed.

The current version has support for TCP disabled, because it is still broken.
The XML-config-feature (for setup go [Edit]->Preferences->TRDP and load your xml file there) now works for much more complex datasets.

U S A G E
=========

The current update was not yet tested on Windows.

On Linux, use your distribution's package manager to install wireshark or wireshark-qt. If it happens to be a version of 2.6.* or 3.0.*, you may be lucky with the pre-compiled plugin-libraries, go:

cp   linux64/2.6/trdp_spy.so   ~/.local/lib/wireshark/plugins/2.6/epan/
cp   linux64/3.0/trdp_spy.so   ~/.local/lib/wireshark/plugins/3.0/epan/

Or copy with a file manager of your choice. Now, when you start wireshark, check in the [Help]-menu --> About Wireshark and open the [Plugins] tab. Somewhere in the middle there should be trdp_spy.so. If not, check the [Folder] tab if your wireshark expects user-plugins somewhere else.


B U I L D I N G
===============

Debian & Ubuntu
---------------

Do this somewhere outside of this tcnopen source tree!!
 __e.g.__  mkdir ~/build && cd ~/build

$ apt source wireshark
$ sudo apt build-dep wireshark
$
$ ### pull in trdp-spy sources
$ cd wireshark-*
$ ln -s path-to-tcnopen-trdp/trdp/spy/src/CMakeListsCustom.txt
$ cd plugins/epan
$ ln -s path-to-tcnopen-trdp/trdp/spy/src/trdp_spy
$ cd ../../
$
$ dpkg-buildpackage -us -uc -rfakeroot   # (give this a few minutes ...)
$ ls */run/plugins/*/epan/trdp_spy.so

This builds all wireshark packages (folder above), which you may need to install. And you should've also found the built trd_spy dissector.

If you don't want / cannot to use dpkg-buildpackage, you can instead, in the root-wireshark-source folder

$ mkdir obj && cd obj
$ cmake ..
$ make trdp_spy

Obviously, if you need to build all of wireshark, call make w/o the target-parameter


Other Linux
-----------

Instead of apt, you can pull the latest stable release from [1], extract it and follow the Debian-example from line 3. When you call cmake, it will most probably tell you, which dependencies you still need to install.

[1] http://www.wireshark.org/download.html


Other OS (eg. Windows)
----------------------

Download the source code from wireshark for the project's homepage [1].
Follow the build instructions for the used operating system [2].

*Copy* the *trdp_spy* folder into the "plugins/epan" subfolder of your wireshark source directory.
*Copy CMakeListsCustom.txt* into the root of the wirshark sources, (next to the plugins folder).

Generate a new folder, as described in the build instructions, next to the source code. E.g. wireshark-custom

Copy (and probalby adapt) the two deliverd bat files from the subfolder scripts into the new generated wireshark-custom folder.
Open there a command prompt and execute the following commands:
> setupEnv.bat
> cmake ../wireshark-2.2.7/
> runIDE.bat

Now Visual Studio is opened, where the complete solution can be rebuilt.

[1] http://www.wireshark.org/download.html
[2] http://www.wireshark.org/docs/wsdg_html_chunked/ChapterSetup.html

Last modified by Thorsten, 2019-April

