                                                               
                  _____ _____ ____  _____    _____ _____ __ __ 
                 |_   _| __  |    \|  _  |  |   __|  _  |  |  |
                   | | |    -|  |  |   __|  |__   |   __|_   _|
                   |_| |__|__|____/|__|     |_____|__|    |_|  
                                                               

This Plugin can be used to display packages containing TRDP (Train Realtime Data Protocol).

You can apply your XML-config-file (for setup go [Edit]->Preferences->Protocols->TRDP and load your xml file there) for filtering and displaying.

N E W S
=======
2021-08 Florian <florian.weispfenning@alstomgroup.com>

 * 	fix for #371 ComIDs > 2147483647 wrong displayed
 
2020-12 Thorsten <t12z@tractionpad.de>

 * Updated handling of Bitset type (TRDP-type 1). Antivalents and Bitsets are displayed correctly now. The fall-back
   handling on numeric type-idents is chosen in the protocol preferences. The config is reloaded automatically.

2020-12 Thorsten <thorsten.schulz@uni-rostock.de>

 * Build for Wireshark 3.4 added. Sources compile w/o any changes.

2020-02 Thorsten <thorsten.schulz@uni-rostock.de>

 * Added deb/dpkg based packaging for Linux.
 
 * Added Makefile for Linux building. Needs libwireshark-dev as dependency and doxygen for HTML-docs.
 
 * Doxygen PDF generation repaired. However, only the HTML output is included in the packaging to minimize build-dep.

2020-01 Thorsten <thorsten.schulz@uni-rostock.de>

 * Build for Wireshark 3.2 added. Sources compile w/o any changes.
 
2019-06 Thorsten <thorsten.schulz@uni-rostock.de>

 * Update on time types representation.
 
 * Time display can be fine-tuned in the TRDP-Spy preferences.
 
2019-04 Thorsten <thorsten.schulz@uni-rostock.de>

 * Support for Wireshark 2.6 and 3.0. It may not work with earlier versions -- checkout older SVN revions if needed.

 * Filtering based on dataset name and element name works!
 
 * Parsing and filtering on strings works.

 * Folding of dataset subtrees is more fine-grained now

 * More details on XML-file parsing issues.

 * QtXML dependency removed

 * TCP may not work, not tested
 
 * Doxygen does not produce the PDF, but you can make it generate HTML (not included here)


U S A G E
=========

On Linux, use your distribution's package manager to install wireshark or wireshark-qt.
On Windows, download from [1].
(Compared to earlier releases, you don't need the QtXML library any more.)
If Wireshark happens to be a version of 2.6.*, 3.0.*, 3.2.* or 3.4.*, you may be lucky with the pre-compiled plugin-libraries. For version 3.4 (Linux) find them here:

cp   plugins/3.4/epan/trdp_spy.so   ~/.local/lib/wireshark/plugins/3.4/epan/

Or copy with a file manager of your choice.
On Windows, obviously, take the *.dll files instead. The folder path for user plugins can be found when running Wireshark, go into [Help]->About Wireshark->Folders and check where "Personal Plugins" should go. You can double click on the path to open your platform's file-manager. Then put it into the sub-folder "epan" and restart Wireshark.

Now, when Wireshark started, check in the [Help]-menu --> About Wireshark and open the [Plugins] tab. Somewhere in the middle there should be trdp_spy.so. If not, check the [Folder] tab if your wireshark expects user-plugins somewhere else.

Again, to make the most of the Wireshark dissector, provide your trdp-xml file in [Edit]->Preferences->Protocols->TRDP. If it cannot dissect your capture data, then you most probably have an issue in that xml file, e.g., buggy XML syntax, misspelled trdp-type or logical errors. A lot of errors will be reported on loading. If the problem persists, check again, just more thoroughly ;)

Within that preference page you can also check whether integer/real values are displayed scaled according to the xml description, or raw. This also has an effect, when you filter for a certain value.
E.g.,
  for an INT32 with scale=0.001:
  [display filter] trdp.pdu.PressureType.current < 9.21
  same xml scale definition, but without the scaling preference option activated:
  [display filter] trdp.pdu.PressureType.current < 9210

___Notice_______________

Due to a shortcoming in the Wireshark-Plugin-API, if you change TRDP-Dissector settings, the display-filter expression is cleared. I have no better solution at the moment. Your last filter expression should, however, still be in the filter history.

___How to filter_________

Currently you can filter on a value of a named Element in a named Dataset (trdp.pdu.datasetName.elementName). If a Dataset has no name (and only then), its number can be used.
For example, select a TRDP packet of your favour, expand the PDU / dataset tree and click on the element of your interest. In the status bar of Wireshark you will see more information on the element, such as the type, scaling, unit, offset, and especially the name, you can use for filtering. You can cascade filter with '&&' and '||' at your pleasure, check Wireshark's manual.
A filter string that matches will always select the whole packet, obviously. So in a complex large packet, you still have to find the matching value yourself.
You cannot filter on hierarchy or on array indices. However, you can filter on a Dataset.Element combined with '&&'-operator and the existence of the parenting datasets,
E.g., 
  The Dataset "BrakeType" has an element named "pressure", which is of custom dataset-type "PressureType".
  [display filter] trdp.pdu.PressureType.current < 9.0 && trdp.pdu.BrakeType.pressure

However, this combined filter string will also match, if PressureType occurs at different locations in the Dataset hierarchy and ANY of those matches the value -- not just the one under the BrakeType.

To clarify, this is NOT supported:
  [display filer] trdp.pdu.BrakeType.pressure.current < 9.0
and it will not be supported in the near future as it has many other implications. It does not scale, since, then any instance of a type needs to be predefined, also for arrays -- this is especially bogous for dynamic arrays.

PS The old filter syntax of trdp-type, e.g., trdp.INT32, is NOT supported anymore.

___Coloring Hint_________

By default wireshark's coloring rules paint packets with low TTL value red (ie. local TRDP packets). You can change or deactivate this rule in [Menu::View]->Coloring Rules->"TTL low or unexpected". I did, it freaked me out.


B U I L D I N G
===============

Debian & Ubuntu
---------------

Either build the deb-packages from the top level of trdp

$ make bindeb-pkg

Or, first install the build dependencies, and then run the Makefile

$ sudo apt install libwireshark-dev libglib2.0-dev
$ make -C path-to-tcnopen-trdp/trdp/spy/src/trdp_spy


Other Linux
-----------

Find out the the names for the build-dependency pacakges and install them, then proceed with make as above.


Windows
-------

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
> msbuild /m /p:Configuration=RelWithDebInfo Plugins.vcxproj


[1] https://www.wireshark.org/download.html
[2] https://www.wireshark.org/docs/wsdg_html_chunked/ChSetupWin32.html#ChWin32Build
[3] https://sourceforge.net/projects/winflexbison/files/

Last modified by Thorsten, 2020-Dec

--
notes to self for rebuilding:
Windows:
# Does not build on net-share anymore (as of 2020-01), must make local copy for building. Otherwise same thing.
