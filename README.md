# TCNopen fork

## Repo is put to rest

As I wrote [before](#in-2021-and-2022):
> "The upstream project still receives active development but focuses on many goals. When I pull their additions, I notice that warning-free Linux compilation is not their primary focus, which also makes keeping up tiring."

So I will pause doing it. I may still add updates to [TRDP-SPY](trdp/spy/) Wireshark plugin here in the master-branch, but don't count on it.

I will also only react to issues concerning the Wireshark plugin and code that I added beyond upstream.

## About 
This is a "private" fork of TCNopen (Components for IEC61375 standardised communication)

**Now awesome**: 
Debian packaging is there! See below for news on 2020.

I rewrote the Wireshark Plugin [TRDP-SPY](trdp/spy/) for current Wireshark. Compiled binaries are available for versions [2.6](trdp/spy/plugins/2.6/epan), [3.0](trdp/spy/plugins/3.0/epan), [3.2](trdp/spy/plugins/3.2/epan), [3.4](trdp/spy/plugins/3.4/epan), [3.6](trdp/spy/plugins/3.6/epan) and [4.0](trdp/spy/plugins/4.0/epan). It now really works for Datasets, Arrays, Strings and filtering on names. Try it out! Create an issue if it doesn't work for you. Please be verbose if I misinterpreted the standard somewhere. From the latest revision (only available for Wireshark 3.6) you can load a folder full of XML-configs.

## Goal of fork
 - The [master](https://github.com/T12z/TCNopen/tree/master) for me to play around.
 - The [upstream](https://github.com/T12z/TCNopen/tree/upstream) if all you need is a mirror.
 - Nothing else, no product, no specific enhancements.

## In 2017
 - I updated the TRDP-Spy plugin to wireshark 2.5. Later on, Upstream have updated the plugin to a 2.x version as well, so I split this off to an [archive branch](https://github.com/T12z/TCNopen/tree/wireshark2.5). 
 - I also started into looking how to pair that up with SCADE but got off-tracked into "paid work". Currently it is *nothing* useful. Don't bother looking. If there is more interest in this topic send me some kind of thumbs-up, so I feel like I should get back onto it.
 
## In 2018
 - So I synced an update, yet haven't really looked into it. But I did trash the branch history, sorry for that!
 - In future, I will try to separate [upstream](https://github.com/T12z/TCNopen/tree/upstream) and my work, so the git-svn shuffling might work smoother.

## In 2019
 - I finally gave up on all my git misery and rebooted the repo. Syncing with upstream became a huge pain and everytime git-bugs piled up. I am **absolutely deeply sorry** for everyone, who had a clone and now has to restart.
 - I built a TRDP application that uses a wrapper around the XML stuff, partially based on the original example.
 - This wrapper is again lightly cling-wrapped to be usable from C++/Qt. Planning to publish it a.s.a.p. (As of writing, it still has too many dependencies on other stuff.)
 - Finally, I also use [Ansys SCADE](https://www.ansys.com/de-de/products/embedded-software/ansys-scade-suite), a lot, and have it working with TRDP nicely. However, if you are not using SCADE, you're in for pain. Currently, I am trying to trim a middle way, please hold on for a few weeks or drop me line.

## In 2020
 Some more Linux features:
 - Debian packaging and shared libraries added for Linux.
 - You now can build the Wireshark plugin on Linux w/o the Wireshark source - just from the spy tree via make or from the root with debuild.
 - How build the Debian packages? Install the build-dependencies (Build-Depends: debhelper (>= 12.8), doxygen, libwireshark-dev (>= 2.6), libglib2.0-dev, graphviz), go into tcnopen-trdp/trdp and run "make bindeb-pkg" to build the lib and the wireshark plugin packages. Install them with "sudo dpkg -i ../*.deb". Wireshark will find its plugin automatically and your Linux compiler should find the headers also. Only need to add -ltrdp -ltau or -ltrdp-hp -ltau-hp to the linker of your applications.
 With this, I will not pre-build the Wireshark plugins anymore. Please checkout the upstream branch for older pre-build plugin binaries 2.6, 3.0 and 3.2.
 - Actually, I added the plugins for 3.4 for convenience for a last time.

## In 2021 and 2022
 I moved jobs and now work for Stadler Rail. I am not using TRDP on any recurring basis anymore in my position (rather MQTT, VDV301 ...). So this repo will wind down even further in updates.
 Some upgrades and bugfixes in the SPY were pushed. Building for Windows is still quite annoying and eats precious time.
 The upstream project still receives active development but focuses on many goals. When I pull their additions, I notice that warning-free Linux compilation is not their primary focus, which also makes keeping up tiring. You'll also notice some [differences](https://github.com/T12z/TCNopen/compare/upstream...master) between [upstream](https://github.com/T12z/TCNopen/tree/upstream) and [master](https://github.com/T12z/TCNopen/tree/master)

## Missing
 - Regular Updates. This is NOT in sync nor latest update from original sourceforge SVN
 - Support. I am no TRDP expert, neither have I project funds to work on TRDP

More information from SourceForge site: https://sourceforge.net/projects/tcnopen/

# Original Description

TCNOpen is an open source initiative which the partner railway industries created with the aim to build in collaboration some key parts of new or upcoming railway standards, commonly known under the name TCN.
TCN (Train Communication Network) is a series of international standards (IEC61375) developed by Working Group 43 of the IEC (International Electrotechnical Commission), specifying a communication system for the data communication within and between vehicles of a train. It is currently in use on many thousands of trains in the world in order to allow electronic devices to exchange information while operating aboard the same train.
TCNOpen follows the Open Source scheme, as the software is jointly developed by participating companies, according to their role, so as to achieve cheaper, quicker and better quality results.

## Licenses

TRDP: MPLv2.0 http://www.mozilla.org/MPL/2.0/ 

TRDPSpy: GPL http://www.gnu.org/licenses/gpl.html 

TCNOpen Web Site http://www.tcnopen.eu/
