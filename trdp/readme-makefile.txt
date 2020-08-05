TCNOpen TRDP Makefile Readme
$Id$

*** Hints for using the Makefile ***

1) Copy and edit a configuration file inside the configuration directory. They are stored in the files ending with
    '_config' (e.g. POSIX_X86_config or VXWORKS_PPC_config).
    These files define the toolchain and (cross-)compiler incl. specific compiler flags to be used.

2) Use 'make MY_TARGET_config'
	which copies the settings for the specified target to 'config/config.mk', which is then included 
	automatically every time make is called. 
	Steps 1-2 need to be done once, only.

3) Use 'make'
    It will use the selected configuration and build most files for your target.

If you want to share your config file:
    Add your *_config to the repository and also add its name to the help-section of the Makefile.

You can use 'make help' to view available parameters and further make information. To build debug versions, you can
call 'make' with the option DEBUG=TRUE.

*** Note: ***
    Since TRDP V2, makefiles regarding 'Ladder' have been moved into the 'ladder' directory!
    To use them, they must be moved back to the trdp top directory, again.

    Most of the files in the config directory are examples and can be used as starting points for your special target!
