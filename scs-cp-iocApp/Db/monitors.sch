[schematic2]
uniq 241
[tools]
[detail]
w 562 2603 100 0 n#236 elongouts.scsstate.VAL 416 2592 768 2592 768 2976 1056 2976 egenSub.scsStateString.INPA
w 642 2219 100 0 n#236 esubs.state1.INPA 736 2208 608 2208 608 2592 junction
w 962 2315 100 0 n#235 esubs.state1.FLNK 1024 2048 1024 2176 928 2176 928 2304 1056 2304 egenSub.scsStateString.SLNK
w 466 2219 100 0 n#227 elongouts.elongouts#98.VAL 416 2208 576 2208 576 2144 736 2144 esubs.state1.INPC
w 530 2251 100 0 n#234 elongouts.elongouts#98.FLNK 416 2240 704 2240 junction
w 530 2635 100 0 n#234 elongouts.scsstate.FLNK 416 2624 704 2624 704 1824 736 1824 esubs.state1.SLNK
w 1220 1467 100 0 n#187 ebis.interlockOverride.VAL 1184 1376 1216 1376 1216 1568 1312 1568 esubs.interlock.INPB
w 1730 2763 100 0 n#185 hwout.hwout#182.outp 1760 2752 1760 2752 egenSub.scanHealth.OUTA
w 1730 2699 100 0 n#184 hwout.hwout#179.outp 1760 2688 1760 2688 egenSub.scanHealth.OUTB
w 1636 1403 100 0 n#173 esubs.interlock.FLNK 1600 1440 1632 1440 1632 1376 1664 1376 ebos.intStatus.SLNK
w 1602 1419 100 0 n#172 esubs.interlock.VAL 1600 1408 1664 1408 ebos.intStatus.DOL
w 1218 1611 100 0 n#116 ebis.setInterlock.VAL 1184 1600 1312 1600 esubs.interlock.INPA
w 2530 2795 100 0 n#104 egenSub.clock.VALA 2464 2784 2656 2784 eaos.eaos#102.DOL
w 2564 2395 100 0 n#103 egenSub.clock.FLNK 2464 2048 2560 2048 2560 2752 2656 2752 eaos.eaos#102.SLNK
f 224 1840 592 2016 -100 256 state
s 2592 3024 100 0 monitors.sch
s 2816 752 100 0 23-Jun-98
s 2544 720 100 0 checked: S.Prior
s 2544 752 100 0 author: S.Prior
s 3152 736 100 0 1
s 3056 736 100 0 1
s 2784 800 100 0 Non-CAD records for housekeeping
s 2784 864 100 0 Secondary Control System
s 240 1968 100 0 Current scs and guide states
s 240 1936 100 0 written to subroutine to
s 240 1904 100 0 make globally available
s 240 1872 100 0 to the CAD routines
[cell use]
use snlStates 2464 967 100 0 snlStates#237
xform 0 2624 1104
use hwout 1760 2711 100 0 hwout#182
xform 0 1856 2752
p 1856 2784 100 0 -1 val(outp):$(top)health.VAL PP NMS
use hwout 1760 2647 100 0 hwout#179
xform 0 1856 2688
p 1856 2640 100 0 -1 val(outp):$(top)health.IMSS
use egenSub 1056 2215 100 0 scsStateString
xform 0 1200 2640
p 1056 2112 100 0 1 DESC:convert scsState into a string
p 833 1989 100 0 0 FTA:LONG
p 833 1989 100 0 0 FTVA:STRING
p 833 1989 100 0 0 FTVB:STRING
p 833 1829 100 0 0 FTVF:LONG
p 833 1829 100 0 0 FTVG:LONG
p 833 1797 100 0 0 FTVH:LONG
p 833 1765 100 0 0 FTVI:LONG
p 833 1733 100 0 0 FTVJ:LONG
p 1056 2064 100 0 1 INAM:scsStateStringInit
p 1056 2032 100 0 1 PV:$(top)
p 1056 2176 100 0 0 SCAN:Passive
p 1056 2144 100 0 1 SNAM:scsStateStringConvert
p 1008 2986 75 0 -1 pproc(INPA):NPP
use egenSub 1472 1991 100 0 scanHealth
xform 0 1616 2416
p 1472 1888 100 0 1 DESC:read health message queue
p 1249 1765 100 0 0 FTVA:STRING
p 1249 1765 100 0 0 FTVB:STRING
p 1472 1840 100 0 1 INAM:readHealthInit
p 1269 1105 100 0 1 PV:$(top)
p 1472 1952 100 0 1 SCAN:1 second
p 1472 1920 100 0 1 SNAM:readHealth
use egenSub 2176 1991 100 0 clock
xform 0 2320 2416
p 1953 1765 100 0 0 FTVB:STRING
p 1953 1733 100 0 0 FTVC:LONG
p 1953 1701 100 0 0 FTVD:LONG
p 2240 1952 100 0 1 INAM:
p 2272 2848 100 0 1 PV:$(top)
p 2240 1888 100 0 1 SCAN:1 second
p 2240 1920 100 0 1 SNAM:ticker
use ebis 976 1264 100 0 interlockOverride
xform 0 1056 1392
p 755 1467 100 0 0 DESC:Set interlock status
p 976 1184 100 0 1 ONAM:ignore gis
p 976 1296 100 0 1 PV:$(top)
p 976 1232 100 0 1 ZNAM:read gis
use ebis 976 1488 100 0 setInterlock
xform 0 1056 1616
p 755 1691 100 0 0 DESC:Set interlock status
p 704 1454 100 0 0 ONAM:active
p 976 1520 100 0 1 PV:$(top)
p 704 1486 100 0 0 ZNAM:not active
use ebos 1664 1287 100 0 intStatus
xform 0 1792 1376
p 1432 1532 100 0 0 DESC:current interlock status
p 1344 1326 100 0 0 OMSL:closed_loop
p 1344 1230 100 0 0 ONAM:ACTIVE
p 1664 1248 100 0 1 PV:$(top)
p 1344 1262 100 0 0 ZNAM:NOT ACTIVE
use showGuides 2048 839 100 0 showGuides#170
xform 0 2208 1040
use tcsSad 2048 1271 100 0 tcsSad#169
xform 0 2208 1480
use elongouts 160 2096 -100 0 elongouts#98
xform 0 288 2208
p 192 2112 100 0 -1 name:$(top)coincidence
use elongouts 160 2503 -100 0 scsstate
xform 0 288 2592
p 288 2672 100 1024 -1 name:$(top)scsState
use esubs 416 1127 100 0 percent
xform 0 560 1392
p 259 1579 100 0 0 DESC:percentage in position
p 416 1040 100 0 1 INAM:initInterlock
p 128 1278 100 0 0 PREC:2
p 416 1008 100 0 1 PV:$(top)
p 416 944 100 0 1 SCAN:.05 second
p 416 1072 100 0 1 SNAM:percentCalc
p 384 1600 75 1280 -1 pproc(INPA):PP
use esubs 736 1735 100 0 state1
xform 0 880 2000
p 736 1664 100 0 1 INAM:stateInit
p 912 1808 100 0 1 PV:$(top)
p 736 1632 100 0 1 SNAM:stateMonitor
p 1152 1824 100 1024 1 name:$(top)state1
use esubs 1312 1127 100 0 interlock
xform 0 1456 1392
p 1155 1579 100 0 0 DESC:interlock mechanism check
p 1312 1008 100 0 1 INAM:initInterlock
p 1312 1088 100 0 1 PV:$(top)
p 1312 960 100 0 1 SCAN:1 second
p 1312 1056 100 0 1 SNAM:lockMonitor
p 1280 1600 75 1280 -1 pproc(INPA):PP
p 1280 1568 75 1280 -1 pproc(INPB):PP
use car 2464 1271 100 0 car#124
xform 0 2624 1480
use eaos 2656 2663 100 0 eaos#102
xform 0 2784 2752
p 2400 2734 100 0 0 OMSL:closed_loop
p 2704 2656 100 0 -1 name:$(top)present
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
