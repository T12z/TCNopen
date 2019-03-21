TCNOpen TRDP Makefile Readme
$Id$

*** Hints for usage of Makefile in build shells ***
1) Edit 'buildsettings_%TARGET%_TEMPLATE' in 'config' folder and change its file name (e.g. to
	'buildsettings_%TARGET%'), so that it is not overwritten on svn update. Adapt the specified 
	paths to your build system environment.

2) Apply buildsettings in build shell using 'source config/buildsettings_%TARGET%'

3) Make configuration settings for target. These are stored in the files ending with '_config' in 
	the 'config' folder (e.g. POSIX_X86_config or VXWORKS_PPC_config). Use 'make POSIX_X86_config'
	which copies the settings for the specified target to 'config/config.mk', which is then included 
	automatically every time make is called. 
	Steps 1-3 need to be done once. Only Step 2 must be repeated every time the shell is changed / closed.

4) Use 'make help' to view available parameters and further make information. As the target has already been
	defined by the config settings in Step 3, no more information needs to be passed to make.
