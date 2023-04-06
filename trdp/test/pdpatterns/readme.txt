TRDP process data test program.

usage: trdp-pd-test <localip> <remoteip> <mcast> <logfile>
where:
  <localip>  .. own IP address (ie. 10.2.24.1)
  <remoteip> .. remote peer IP address (ie. 10.2.24.2)
  <mcast>    .. multicast group address (ie. 239.2.24.1)
  <logfile>  .. optional name of the logfile (ie. log.txt)

The purpose of this test program is to verify TRDP PD transmission between
two hosts using all defined communication patterns.

Two separate host computers are needed to run the test and one multicast
group address from local scope (ie. 239....). Example:

hostA: 10.2.24.1
hostB: 10.2.24.2

On hostA, use this command to start the test:

  ./trdp-pd-test 10.2.24.1 10.2.24.2 239.2.24.1 log.txt

On hostB, use this command to start the test:

  ./trdp-pd-test 10.2.24.2 10.2.24.1 239.2.24.1 log.txt


Test creates these datasets with PUSH pattern
(both publish/subscribe on each side):

The exact period depends on the high performance index table base.
Standard with HIGH_PERF_INDEXED=1 is base 10: 1, 10, 100ms.
With additional HIGH_PERF_BASE2=1 the index is switched to base 2: 1, 8, 64ms.

<comid>  <destination>  <size>  <period>     <data>
 10046    unicast        256b   100/128ms     generated
 20046    unicast        256b   100/128ms     copied from incoming 10046
 10086    unicast        256b   250/256ms     generated
 20086    unicast        256b   250/256ms     copied from incoming 10086
 10049    unicast       1432b   100/128ms     generated
 20049    unicast       1432b   100/128ms     copied from incoming 10049
 10089    unicast       1432b   250/256ms     generated
 20089    unicast       1432b   250/256ms     copied from incoming 10089

 10146    multicast      256b   100/128ms     generated
 20146    multicast      256b   100/128ms     copied from incoming 10146
 10186    multicast      256b   250/256ms     generated
 20186    multicast      256b   250/256ms     copied from incoming 10186
 10149    multicast     1432b   100/128ms     generated
 20149    multicast     1432b   100/128ms     copied from incoming 10149
 10189    multicast     1432b   250/256ms     generated
 20189    multicast     1432b   250/256ms     copied from incoming 10189
 
Data generated into every dataset are refreshed every 500 msec on incremented
offset using this pattern:

  <Pd/srcip->dstip/msec/bytes:cycle>
  
Rest of the data are filled by character '_'. When mirroring data from incoming
dataset to outgoing one, these underscore characters are replaced by '~'.


Test creates these datasets with PULL pattern
(both request/subscribe or publish/subscribe on each side):

<comid>  <type>    <repcomid> <destination> <size>  <data>
 30006    request   40006      unicast       256b    generated
 30009    request   40009      unicast      1432b    generated
 30106    request   40106      multicast     256b    generated
 30109    request   40109      multicast    1432b    generated

 40006    reply     -          -             256b    copied from incoming 30006
 40009    reply     -          -            1432b    copied from incoming 30009
 40106    reply     -          -             256b    copied from incoming 30106
 40109    reply     -          -            1432b    copied from incoming 30109


After initialization phase the test periodically draws data and status on
the terminal screen. The screen layout is this:

<comid> <dir>
   <msgtype>  <data>                                                   <status>
--------------------------------------------------------------------------------
10046 Pd -> [_________________<Pd/10.2.24.2->10.2.24.1/100ms/256b:17>.__]   0
20046    <- [~~~~~~~~~~~~~~~<Pd/10.2.24.2->10.2.24.1/100ms/256b:15>.~~~~]   0
10086 Pd -> [_________________<Pd/10.2.24.2->10.2.24.1/250ms/256b:17>.__]   0
20086    <- [~~~~~~~~~~~~~~~<Pd/10.2.24.2->10.2.24.1/250ms/256b:15>.~~~~]   0
10049 Pd -> [_________________<Pd/10.2.24.2->10.2.24.1/100ms/1432b:17>._]   0
20049    <- [~~~~~~~~~~~~~~~<Pd/10.2.24.2->10.2.24.1/100ms/1432b:15>.~~~]   0
10089 Pd -> [_________________<Pd/10.2.24.2->10.2.24.1/250ms/1432b:17>._]   0
20089    <- [~~~~~~~~~~~~~~~<Pd/10.2.24.2->10.2.24.1/250ms/1432b:15>.~~~]   0
10146 Pd -> [_________________<Pd/10.2.24.2->239.2.24.1/100ms/256b:17>._]   0
20146    <- [~~~~~~~~~~~~~~~<Pd/10.2.24.2->239.2.24.1/100ms/256b:15>.~~~]   0
10186 Pd -> [_________________<Pd/10.2.24.2->239.2.24.1/250ms/256b:17>._]   0
20186    <- [~~~~~~~~~~~~~~~<Pd/10.2.24.2->239.2.24.1/250ms/256b:15>.~~~]   0
10149 Pd -> [_________________<Pd/10.2.24.2->239.2.24.1/100ms/1432b:17>.]   0
20149    <- [~~~~~~~~~~~~~~~<Pd/10.2.24.2->239.2.24.1/100ms/1432b:15>.~~]   0
10189 Pd -> [_________________<Pd/10.2.24.2->239.2.24.1/250ms/1432b:17>.]   0
20189    <- [~~~~~~~~~~~~~~~<Pd/10.2.24.2->239.2.24.1/250ms/1432b:15>.~~]   0
10046    <- [_____________________________<Pd/10.2.24.1->10.2.24.2/100ms]   0
20046 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pd/10.2.24.1->10.2.24.2/100ms]   0
10086    <- [_____________________________<Pd/10.2.24.1->10.2.24.2/250ms]   0
20086 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pd/10.2.24.1->10.2.24.2/250ms]   0
10049    <- [_____________________________<Pd/10.2.24.1->10.2.24.2/100ms]   0
20049 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pd/10.2.24.1->10.2.24.2/100ms]   0
10089    <- [_____________________________<Pd/10.2.24.1->10.2.24.2/250ms]   0
20089 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pd/10.2.24.1->10.2.24.2/250ms]   0
10146    <- [_____________________________<Pd/10.2.24.1->239.2.24.1/100m]   0
20146 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pd/10.2.24.1->239.2.24.1/100m]   0
10186    <- [_____________________________<Pd/10.2.24.1->239.2.24.1/250m]   0
20186 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pd/10.2.24.1->239.2.24.1/250m]   0
10149    <- [_____________________________<Pd/10.2.24.1->239.2.24.1/100m]   0
20149 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pd/10.2.24.1->239.2.24.1/100m]   0
10189    <- [_____________________________<Pd/10.2.24.1->239.2.24.1/250m]   0
20189 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pd/10.2.24.1->239.2.24.1/250m]   0
40006    <- [~~~~~~~~~~~~~~~<Pr/10.2.24.2->10.2.24.1/0ms/256b:15>.~~~~~~]   0
30006 Pr -> [_________________<Pr/10.2.24.2->10.2.24.1/0ms/256b:17>.____]   0
40009    <- [~~~~~~~~~~~~~~~<Pr/10.2.24.2->10.2.24.1/0ms/1432b:15>.~~~~~]   0
30009 Pr -> [_________________<Pr/10.2.24.2->10.2.24.1/0ms/1432b:17>.___]   0
40106    <- [~~~~~~~~~~~~~~~<Pr/10.2.24.2->239.2.24.1/0ms/256b:15>.~~~~~]   0
30106 Pr -> [_________________<Pr/10.2.24.2->239.2.24.1/0ms/256b:17>.___]   0
40109    <- [~~~~~~~~~~~~~~~<Pr/10.2.24.2->239.2.24.1/0ms/1432b:15>.~~~~]   0
30109 Pr -> [_________________<Pr/10.2.24.2->239.2.24.1/0ms/1432b:17>.__]   0
30006    <- [_____________________________<Pr/10.2.24.1->10.2.24.2/0ms/2]   0
40006 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pr/10.2.24.1->10.2.24.2/0ms/2]   0
30009    <- [_____________________________<Pr/10.2.24.1->10.2.24.2/0ms/1]   0
40009 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pr/10.2.24.1->10.2.24.2/0ms/1]   0
30106    <- [_____________________________<Pr/10.2.24.1->239.2.24.1/0ms/]   0
40106 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pr/10.2.24.1->239.2.24.1/0ms/]   0
30109    <- [_____________________________<Pr/10.2.24.1->239.2.24.1/0ms/]   0
40109 Pd -> [~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<Pr/10.2.24.1->239.2.24.1/0ms/]   0
--------------------------------------------------------------------------------

The <status> column shows last return code from the tlp_get/tlp_put/tlp_request
function call for given dataset. It is printed in green color when the
status is 0 (TRDP_NO_ERR) and in red color if not.

You should see the zero status (green) if the communication works as expected.

ComIDs in blue color show multicast addressed traffic - either receiving or sending.
