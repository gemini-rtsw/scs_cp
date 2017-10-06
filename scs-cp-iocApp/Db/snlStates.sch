[schematic2]
uniq 256
[tools]
[detail]
w 402 2635 100 0 n#255 elongouts.cadProcessorState.FLNK 416 2624 448 2624 448 2336 junction
w 402 2347 100 0 n#255 elongouts.followDemandState.FLNK 416 2336 448 2336 448 2048 junction
w 402 2059 100 0 n#255 elongouts.elongouts#241.FLNK 416 2048 448 2048 448 1760 junction
w 530 1771 100 0 n#255 elongouts.monitorProcessState.FLNK 416 1760 704 1760 704 1664 junction
w 530 1483 100 0 n#255 elongouts.rebootScsState.FLNK 416 1472 704 1472 junction
w 530 1227 100 0 n#255 esubs.state2.SLNK 800 1664 704 1664 704 1216 416 1216 elongouts.moveBaffleState.FLNK
w 1042 1963 100 0 n#254 egenSub.snlStateString.SLNK 1056 2304 1024 2304 1024 1952 1120 1952 1120 1888 1088 1888 esubs.state2.FLNK
w 658 1899 100 0 n#253 elongouts.moveBaffleState.VAL 416 1184 576 1184 576 1888 800 1888 esubs.state2.INPF
w 770 2123 100 0 n#253 junction 672 1888 672 2112 928 2112 928 2656 1056 2656 egenSub.snlStateString.INPF
w 738 2155 100 0 n#252 junction 640 1920 640 2144 896 2144 896 2720 1056 2720 egenSub.snlStateString.INPE
w 642 1931 100 0 n#252 elongouts.rebootScsState.VAL 416 1440 544 1440 544 1920 800 1920 esubs.state2.INPE
w 706 2187 100 0 n#249 junction 608 1952 608 2176 864 2176 864 2784 1056 2784 egenSub.snlStateString.INPD
w 626 1963 100 0 n#249 elongouts.monitorProcessState.VAL 416 1728 512 1728 512 1952 800 1952 esubs.state2.INPD
w 626 2219 100 0 n#248 junction 480 2016 480 2208 832 2208 832 2848 1056 2848 egenSub.snlStateString.INPC
w 626 1995 100 0 n#248 elongouts.elongouts#241.VAL 416 2016 512 2016 512 1984 800 1984 esubs.state2.INPC
w 642 2315 100 0 n#246 junction 544 2304 800 2304 800 2912 1056 2912 egenSub.snlStateString.INPB
w 642 2027 100 0 n#246 elongouts.followDemandState.VAL 416 2304 544 2304 544 2016 800 2016 esubs.state2.INPB
w 658 2059 100 0 n#236 junction 576 2592 576 2048 800 2048 esubs.state2.INPA
w 562 2603 100 0 n#236 elongouts.cadProcessorState.VAL 416 2592 768 2592 768 2976 1056 2976 egenSub.snlStateString.INPA
s 2784 864 100 0 Secondary Control System
s 2784 800 100 0 Non-CAD records for housekeeping
s 3056 736 100 0 1
s 3152 736 100 0 1
s 2544 752 100 0 author: D.Kotturi
s 2544 720 100 0 checked: D.Kotturi
s 2816 752 100 0 28-Jan-2000
s 2592 3024 100 0 snlStates.sch
[cell use]
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
p -32 632 100 0 0 revision:2.2
use esubs 800 1575 100 0 state2
xform 0 944 1840
p 864 1424 100 0 1 DESC:make states globally available
p 864 1520 100 0 1 INAM:snlStateInit
p 864 1472 100 0 1 SNAM:snlStateMonitor
use elongouts 160 1095 -100 0 moveBaffleState
xform 0 288 1184
p 288 1264 100 1024 -1 name:$(top)moveBaffleState
use elongouts 160 1351 -100 0 rebootScsState
xform 0 288 1440
p 288 1520 100 1024 -1 name:$(top)rebootScsState
use elongouts 160 1639 -100 0 monitorProcessState
xform 0 288 1728
p 288 1808 100 1024 -1 name:$(top)monitorProcessState
use elongouts 160 1927 -100 0 elongouts#241
xform 0 288 2016
p 288 2096 100 1024 -1 name:$(top)monitorSadState
use elongouts 160 2215 -100 0 followDemandState
xform 0 288 2304
p 288 2384 100 1024 -1 name:$(top)followDemandState
use elongouts 160 2503 -100 0 cadProcessorState
xform 0 288 2592
p 288 2672 100 1024 -1 name:$(top)cadProcessorState
use egenSub 1056 2215 100 0 snlStateString
xform 0 1200 2640
p 1056 2112 100 0 1 DESC:convert enum SNL state into a string
p 833 1989 100 0 0 FTA:LONG
p 833 1989 100 0 0 FTB:LONG
p 833 1957 100 0 0 FTC:LONG
p 833 1925 100 0 0 FTD:LONG
p 833 1893 100 0 0 FTE:LONG
p 833 1829 100 0 0 FTF:LONG
p 833 1989 100 0 0 FTVA:STRING
p 833 1989 100 0 0 FTVB:STRING
p 833 1957 100 0 0 FTVC:STRING
p 833 1925 100 0 0 FTVD:STRING
p 833 1893 100 0 0 FTVE:STRING
p 833 1829 100 0 0 FTVF:STRING
p 1056 2064 100 0 1 INAM:snlStateStringInit
p 1056 2016 100 0 1 PV:$(top)
p 1056 2176 100 0 0 SCAN:Passive
p 1056 2144 100 0 1 SNAM:snlStateStringConvert
p 1008 2986 75 0 -1 pproc(INPA):NPP
[comments]
