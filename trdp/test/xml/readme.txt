trdp-xmlprint-test
------------------
Parses all available configuration data from provided XML configuration file
into TRDP structures.
Prints content of all structures with parsed configuration data.

Usage:
    trdp-xmlprint-test <cfgFileName>
    
trdp-xmlpd-test
---------------
Configures TRDP PD according to the supplied XML configuration file, 
send and receives telegrams, prints out the contet of sent and received data.
Following general steps are performed:
  - General configuration parameters are read from XML file using tau_readXmlConfig
  - TRDP stack is initialized with configured memory structure
  - Dataset configuration is read using tau_readXmlDatasetConfig
  - TRDP marshalling is initialized using dataset configuration
  - One session for each configured interface is initialized
  - Telegrams are configured for each session (interface). 
    When some parameters are missing, default values are used.
    - Telegram is published for each configured destination 
     (IP address expected in destination URI).
    - Telegram is subscribed for each configured source. 
      IP addresses are expected in source URI parameters. 
      First multicast address found among telegram destinations (if any) 
      is used as destination address.
  - Periodically telegrams are sent and received:
    - All dataset elements in published telegrams are set to value of global counter
      (incremented each period). Datatype of each element is taken into account 
      (TIMEDATE types not yet supported).
    - Updated datasets are passed to TRDP stack using tlp_put.
    - tlc_process is called for each configured session.
    - Received telegrams are read using tlp_get.
    - Content of all sent and received telegrams is printed out. 
      Dataset elements are printed according to their data type.
  
Usage:
    trdp-xmlpd-test <cfgFileName>
  
  