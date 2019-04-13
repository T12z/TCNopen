                                                               
                  _____ _____ ____  _____    _____ _____ __ __ 
                 |_   _| __  |    \|  _  |  |   __|  _  |  |  |
                   | | |    -|  |  |   __|  |__   |   __|_   _|
                   |_| |__|__|____/|__|     |_____|__|    |_|  
                                                               

															   
This Plugin can be used to display packages containing TRDP (Train Realtime Data Protocol).

The Plugin was previously developed for wireshark version 2.2.7.
However, the current version was tested with Wireshark 2.6 and 3.0. It may not work with earlier versions -- checkout older SVN revions if needed.

The current version has support for TCP disabled, because it is still broken.
The XML-config-feature (for setup go [Edit]->Preferences->Protocols->TRDP and load your xml file there) now works for much more complex datasets.

U S A G E
=========

On Linux, use your distribution's package manager to install wireshark or wireshark-qt. On Windows, download from [1].
If it happens to be a version of 2.6.* or 3.0.*, you may be lucky with the pre-compiled plugin-libraries, go (Linux):

cp   plugins/2.6/epan/trdp_spy.so   ~/.local/lib/wireshark/plugins/2.6/epan/
cp   plugins/3.0/epan/trdp_spy.so   ~/.local/lib/wireshark/plugins/3.0/epan/

Or copy with a file manager of your choice.
On Windows, obviously, take the *.dll files. (see ./win64/readme.txt)
Now, when you start wireshark, check in the [Help]-menu --> About Wireshark and open the [Plugins] tab. Somewhere in the middle there should be trdp_spy.so. If not, check the [Folder] tab if your wireshark expects user-plugins somewhere else.

Again, to make the most of the Wireshark Dissector, provide your trdp-xml file in [Edit]->Preferences->Protocols->TRDP. If it cannot dissect your capture data, then you most probably have an issue in that xml file, e.g. buggy XML syntax, misspelled trdp-type or logical errors.


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


Windows
-------

Actually, there is some similarity to the Linux approach, thanks to CMake, and in fact, you could build Linux and Windows binaries from the same prepared source tree! Anyways,

Download the source code from wireshark for the project's homepage [1].
Follow the wireshark preparation instructions [2], resembling the following:

Install MS C+SDK, Qt, Python, Perl, CMake, WinFlexBison [3], as described in [2].
For WinFlexBison, unpack it somewhere reasonable and add its path to Windows %path% environment variable. Remember throughout the installation that Python, Perl and CMake should also be found through the %path% variable.

*Copy* the *trdp_spy* folder into the "plugins/epan" subfolder of your wireshark source directory.
*Copy CMakeListsCustom.txt* into the root of the wirshark sources, (next to the plugins folder).

For the following, I expect the VC++ tools to be version 2017 and Qt to be 5.12.2. Adapt, as you see differences.
Open the VS2017 prompt (search Start menu for "x64 Native Tools [..]") and change into the root of your extracted+prepared wireshark sources.

> set WIRESHARK_BASE_DIR=%CD%
> mkdir obj-x86_64-win
> cd obj-x86_64-win
> set QT5_BASE_DIR=C:\Qt\5.12.2\msvc2017_64
> cmake -G "Visual Studio 15 2017 Win64" ..
> msbuild /m /p:Configuration=RelWithDebInfo Wireshark.sln


[1] https://www.wireshark.org/download.html
[2] https://www.wireshark.org/docs/wsdg_html_chunked/ChSetupWin32.html#ChWin32Build
[3] https://sourceforge.net/projects/winflexbison/files/

Last modified by Thorsten, 2019-April

--
notes to self for rebuilding:
Windows:
set QT5_BASE_DIR=C:\Qt\5.12.2\msvc2017_64
P:
cd P:\build\wireshark-3.0*
set WIRESHARK_BASE_DIR=%CD%
cd obj-x86_64-win
msbuild /m /p:Configuration=RelWithDebInfo Wireshark.sln
cd P:\build\wireshark-2.6*
set WIRESHARK_BASE_DIR=%CD%
cd obj-x86_64-win
msbuild /m /p:Configuration=RelWithDebInfo Wireshark.sln

Linux:
cd ~/Projekte/build/wireshark-2.6*/obj-x86_64-linux-gnu && make trdp_spy
cd ~/Projekte/build/wireshark-3.0*/obj-x86_64-linux-gnu && make trdp_spy

cp ~/Projekte/build/wireshark-2.6*/obj-x86_64-linux-gnu/run/plugins/*/epan/trdp_spy.so plugins/2.6/epan/
cp ~/Projekte/build/wireshark-2.6*/obj-x86_64-win/run/RelWithDebInfo/plugins/*/epan/trdp_spy.dll plugins/2.6/epan/
cp ~/Projekte/build/wireshark-3.0*/obj-x86_64-linux-gnu/run/plugins/*/epan/trdp_spy.so plugins/3.0/epan/
cp ~/Projekte/build/wireshark-3.0*/obj-x86_64-win/run/RelWithDebInfo/plugins/*/epan/trdp_spy.dll plugins/3.0/epan/

cd ~/Projekte/tcnopen-trdp/
# git commit -a trdp/spy

