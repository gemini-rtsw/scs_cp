[schematic2]
uniq 166
[tools]
[detail]
w 1250 2667 100 0 n#165 hwin.hwin#137.in 1280 2656 1280 2656 egenSub.storeConfig.INPA
w 1538 2283 100 0 n#164 hwout.hwout#153.outp 1568 2272 1568 2272 egenSub.storeConfig.OUTG
w 1538 2347 100 0 n#163 hwout.hwout#151.outp 1568 2336 1568 2336 egenSub.storeConfig.OUTF
w 1538 2411 100 0 n#162 hwout.hwout#149.outp 1568 2400 1568 2400 egenSub.storeConfig.OUTE
w 1538 2475 100 0 n#161 hwout.hwout#147.outp 1568 2464 1568 2464 egenSub.storeConfig.OUTD
w 1538 2539 100 0 n#160 hwout.hwout#145.outp 1568 2528 1568 2528 egenSub.storeConfig.OUTC
w 1538 2603 100 0 n#159 hwout.hwout#143.outp 1568 2592 1568 2592 egenSub.storeConfig.OUTB
w 1538 2667 100 0 n#158 hwout.hwout#141.outp 1568 2656 1568 2656 egenSub.storeConfig.OUTA
w 1154 1995 100 0 n#157 inhier.slnk.P 1088 1984 1280 1984 egenSub.storeConfig.SLNK
s 2784 864 100 0 Secondary Control System
s 2768 800 100 0 store previous guide configuration
s 3056 736 100 0 1
s 3152 736 100 0 1
s 2544 752 100 0 author: S.Prior
s 2544 720 100 0 checked: S.Prior
s 2816 752 100 0 01-Mar-98
s 2592 3024 100 0 lookup.sch
[cell use]
use egenSub 1280 1895 100 0 storeConfig
xform 0 1424 2320
p 1123 2635 100 0 0 DESC:guide config values
p 1057 1669 100 0 0 FTA:LONG
p 1057 1669 100 0 0 FTVA:STRING
p 1057 1669 100 0 0 FTVB:STRING
p 1057 1637 100 0 0 FTVC:STRING
p 1057 1605 100 0 0 FTVD:STRING
p 1057 1573 100 0 0 FTVE:STRING
p 1057 1509 100 0 0 FTVF:STRING
p 1057 1509 100 0 0 FTVG:STRING
p 1296 1792 100 0 1 INAM:dummyInitGenSub
p 992 2446 100 0 0 PREC:2
p 1344 1824 100 0 1 PV:$(top)
p 1296 1728 100 0 1 SNAM:lookupGuide
use hwout 1568 2231 100 0 hwout#153
xform 0 1664 2272
p 1664 2263 100 0 -1 val(outp):$(top)guideConfig.I
use hwout 1568 2295 100 0 hwout#151
xform 0 1664 2336
p 1664 2327 100 0 -1 val(outp):$(top)guideConfig.H
use hwout 1568 2359 100 0 hwout#149
xform 0 1664 2400
p 1664 2391 100 0 -1 val(outp):$(top)guideConfig.G
use hwout 1568 2423 100 0 hwout#147
xform 0 1664 2464
p 1664 2455 100 0 -1 val(outp):$(top)filterType PP NMS
use hwout 1568 2487 100 0 hwout#145
xform 0 1664 2528
p 1664 2519 100 0 -1 val(outp):$(top)guideConfig.E
use hwout 1568 2551 100 0 hwout#143
xform 0 1664 2592
p 1664 2583 100 0 -1 val(outp):$(top)guideConfig.D
use hwout 1568 2615 100 0 hwout#141
xform 0 1664 2656
p 1664 2647 100 0 -1 val(outp):$(top)guideConfig.B
use hwin 1088 2615 100 0 hwin#137
xform 0 1184 2656
p 1091 2648 100 0 -1 val(in):$(top)source
use inhier 1072 1943 100 0 slnk
xform 0 1088 1984
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
