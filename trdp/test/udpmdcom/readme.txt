(29/11/2012 - s.pachera, FAR Systems s.p.a.)
Add partial multicast test support (Notify, Request, Reply without confirmation)
To execute UDPMDCom test use 3 devices Dev1 (IP1), Dev2 (IP2) e Dev2 (IP3).


On Dev1
	./mdTest0001 --testmode 1 --dest <IP2>
	A command line interface will be shown
		Type "h" and return to get available commands
		To execute test type related char and next return
		Result will be shown on Dev1, Dev2 and Dev3 console.

On Dev2 (used for singlecast and multicast test)
	./mdTest0001 --testmode 2


On Dev2 (used only for multicast test)
	./mdTest0001 --testmode 3
