TRDP SPY
----------------------------------
Requirements:
Wireshark version 2.6, 3.0 (see sub folders in ../plugins)

Features:
1. TRDP header parser
2. TRDP XML parser

Installation:
Copy Qt5Xml.dll into wireshark's program folder. (You will see other Qt5*.dll there)
Copy trdp_spy.dll from ../plugins/3.0/epan/ into wireshark/plugins/*/epan/ directory. Or, run Wireshark, go into [Help]->About->Folders and check where user-plugins should go. You can double click on the path to open your platform's file-manager.
