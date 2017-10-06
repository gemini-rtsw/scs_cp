[schematic2]
uniq 154
[tools]
[detail]
w 498 1643 100 0 n#135 ebos.focuspidon.FLNK 416 1696 480 1696 480 1632 576 1632 estringouts.focuspidonStr.SLNK
w 498 2763 100 0 n#134 ebos.tiltpidon.FLNK 416 2816 480 2816 480 2752 576 2752 estringouts.tiltpidonStr.SLNK
w 810 1163 100 0 n#132 eseqs.focuspidControlSeq.LNK1 768 1152 912 1152 ecars.focusPidC.IVAL
w 786 1131 100 0 n#132 eseqs.focuspidControlSeq.LNK2 768 1120 864 1120 864 1152 junction
w 452 1115 100 2 n#131 eseqs.focuspidControlSeq.DOL2 448 1120 448 1120 hwin.hwin#130.in
w 452 1147 100 2 n#129 eseqs.focuspidControlSeq.DOL1 448 1152 448 1152 hwin.hwin#128.in
w 770 1243 100 0 n#127 ecad2.focuspidControl.STLK 1280 1328 1440 1328 1440 1232 160 1232 160 832 448 832 eseqs.focuspidControlSeq.SLNK
w 866 1627 100 0 n#126 estringouts.focuspidonStr.OUT 832 1616 960 1616 ecad2.focuspidControl.A
w 466 1675 100 0 n#125 ebos.focuspidon.VAL 416 1664 576 1664 estringouts.focuspidonStr.DOL
w 1236 923 100 2 n#124 ecars.focusPidC.FLNK 1232 928 1232 928 hwout.hwout#123.outp
w 810 2283 100 0 n#117 eseqs.tiltpidControlSeq.LNK1 768 2272 912 2272 ecars.tiltPidC.IVAL
w 786 2251 100 0 n#117 eseqs.tiltpidControlSeq.LNK2 768 2240 864 2240 864 2272 junction
w 452 2235 100 2 n#116 hwin.hwin#115.in 448 2240 448 2240 eseqs.tiltpidControlSeq.DOL2
w 452 2267 100 2 n#114 hwin.hwin#113.in 448 2272 448 2272 eseqs.tiltpidControlSeq.DOL1
w 770 2363 100 0 n#112 ecad2.tiltpidControl.STLK 1280 2448 1440 2448 1440 2352 160 2352 160 1952 448 1952 eseqs.tiltpidControlSeq.SLNK
w 866 2747 100 0 n#110 estringouts.tiltpidonStr.OUT 832 2736 960 2736 ecad2.tiltpidControl.A
w 466 2795 100 0 n#109 ebos.tiltpidon.VAL 416 2784 576 2784 estringouts.tiltpidonStr.DOL
w 1236 2043 100 2 n#107 hwout.hwout#106.outp 1232 2048 1232 2048 ecars.tiltPidC.FLNK
w 2936 1483 100 0 n#90 eseqs.setcontrollerSeq.LNK2 2912 1472 3008 1472 3008 1504 junction
w 2500 2219 100 0 n#90 eseqs.setcontrollerSeq.LNK1 2912 1504 3008 1504 3008 1632 2496 1632 2496 2816 2624 2816 ecars.setControllerC.IVAL
w 2568 1515 100 0 n#86 hwin.hwin#82.in 2592 1504 2592 1504 eseqs.setcontrollerSeq.DOL1
w 2568 1483 100 0 n#85 hwin.hwin#84.in 2592 1472 2592 1472 eseqs.setcontrollerSeq.DOL2
w 2424 1195 100 0 n#81 ecad20.setController.STLK 2304 1184 2592 1184 eseqs.setcontrollerSeq.SLNK
w 1928 1995 100 0 n#79 ebos.guideMode.OUT 1920 1984 1984 1984 ecad20.setController.K
w 2920 2603 100 0 n#76 hwout.hwout#74.outp 2944 2592 2944 2592 ecars.setControllerC.FLNK
s 2592 3024 100 0 $Id: pid.sch,v 1.3 2015/02/03 17:45:20 mrippa Exp $
s 2816 752 100 0 06-Mar-00
s 2544 720 100 0 checked: D.Kotturi
s 2544 752 100 0 author: D.Kotturi
s 3152 736 100 0 1
s 3056 736 100 0 1
s 2784 800 100 0 PID Control
s 2784 864 100 0 Secondary Control System
s 160 1792 200 0 Focus PID Control
s 160 2912 200 0 Tilt PID Control
s 1664 2912 200 0 PID Settings
[cell use]
use hwin 2400 1431 100 0 hwin#84
xform 0 2496 1472
p 2403 1464 100 0 -1 val(in):$(CAR_IDLE)
use hwin 2400 1463 100 0 hwin#82
xform 0 2496 1504
p 2403 1496 100 0 -1 val(in):$(CAR_BUSY)
use hwin 256 2231 100 0 hwin#113
xform 0 352 2272
p 259 2264 100 0 -1 val(in):$(CAR_BUSY)
use hwin 256 2199 100 0 hwin#115
xform 0 352 2240
p 259 2232 100 0 -1 val(in):$(CAR_IDLE)
use hwin 256 1111 100 0 hwin#128
xform 0 352 1152
p 259 1144 100 0 -1 val(in):$(CAR_BUSY)
use hwin 256 1079 100 0 hwin#130
xform 0 352 1120
p 259 1112 100 0 -1 val(in):$(CAR_IDLE)
use hwout 2944 2551 100 0 hwout#74
xform 0 3040 2592
p 2976 2528 100 0 -1 val(outp):$(top)allCar.VAL
use hwout 1232 2007 100 0 hwout#106
xform 0 1328 2048
p 1328 2039 100 0 -1 val(outp):$(top)allCar.VAL
use hwout 1232 887 100 0 hwout#123
xform 0 1328 928
p 1328 919 100 0 -1 val(outp):$(top)allCar.VAL
use ecars 2624 2535 100 0 setControllerC
xform 0 2784 2704
p 2612 2284 100 0 0 PV:$(top)
p 2688 2880 100 0 -1 name:$(top)setControllerC
use ecars 912 1991 100 0 tiltPidC
xform 0 1072 2160
use ecars 912 871 100 0 focusPidC
xform 0 1072 1040
use eseqs 2592 1095 100 0 setcontrollerSeq
xform 0 2752 1344
p 2672 1024 100 0 0 DLY2:1
p 2656 1072 100 0 1 PV:$(top)
p 2560 1504 75 1280 -1 pproc(DOL1):NPP
p 2560 1472 75 1280 -1 pproc(DOL2):NPP
p 2928 1504 75 1024 -1 pproc(LNK1):PP
p 2928 1472 75 1024 -1 pproc(LNK2):PP
use eseqs 448 1863 100 0 tiltpidControlSeq
xform 0 608 2112
p 544 2048 100 0 1 DLY2:1.0e+00
p 784 2272 75 1024 -1 pproc(LNK1):PP
p 784 2240 75 1024 -1 pproc(LNK2):PP
use eseqs 448 743 100 0 focuspidControlSeq
xform 0 608 992
p 544 944 100 0 1 DLY2:1.0e+00
p 784 1152 75 1024 -1 pproc(LNK1):PP
p 784 1120 75 1024 -1 pproc(LNK2):PP
use ecad2 960 2359 100 0 tiltpidControl
xform 0 1120 2672
p 1024 2384 100 0 1 DESC:Tilt PID on/off control
p 1056 2672 100 0 1 FTVA:LONG
p 1040 2608 100 0 1 SNAM:CADtiltPidControl
use ecad2 960 1239 100 0 focuspidControl
xform 0 1120 1552
p 1024 1264 100 0 1 DESC:Tilt PID on/off control
p 1072 1440 100 0 1 FTVA:LONG
p 1040 1488 100 0 1 SNAM:CADfocusPidControl
use estringouts 576 2679 100 0 tiltpidonStr
xform 0 704 2752
p 512 2640 100 0 1 DESC:Tilt PID control parameter
p 512 2608 100 0 1 OMSL:closed_loop
use estringouts 576 1559 100 0 focuspidonStr
xform 0 704 1632
p 512 1520 100 0 1 DESC:Tilt PID control parameter
p 512 1488 100 0 1 OMSL:closed_loop
use ebos 1664 1927 100 0 guideMode
xform 0 1792 2016
p 1432 2172 100 0 0 DESC:Select guiding mode
p 1664 1792 100 0 1 ONAM:PROJECTED
p 1728 1888 100 0 1 PV:$(top)
p 1664 1840 100 0 1 ZNAM:AUTOGUIDE
use ebos 160 2695 100 0 tiltpidon
xform 0 288 2784
p 224 2624 100 0 1 ONAM:ON
p 224 2656 100 0 1 ZNAM:OFF
use ebos 160 1575 100 0 focuspidon
xform 0 288 1664
p 224 1504 100 0 1 ONAM:ON
p 224 1536 100 0 1 ZNAM:OFF
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
use ecad20 1984 1095 100 0 setController
xform 0 2144 1984
p 2144 3272 100 0 0 DESC:Tilt system servo gains
p 2144 3016 100 0 0 FTVA:DOUBLE
p 2144 2984 100 0 0 FTVB:DOUBLE
p 2144 2952 100 0 0 FTVC:DOUBLE
p 2144 2920 100 0 0 FTVD:DOUBLE
p 2144 2888 100 0 0 FTVE:DOUBLE
p 2144 2856 100 0 0 FTVF:DOUBLE
p 2144 2824 100 0 0 FTVG:DOUBLE
p 2144 2792 100 0 0 FTVH:DOUBLE
p 2144 2760 100 0 0 FTVI:DOUBLE
p 2144 2728 100 0 0 FTVJ:DOUBLE
p 2144 2696 100 0 0 FTVK:DOUBLE
p 2144 2664 100 0 0 FTVL:DOUBLE
p 2144 2632 100 0 0 FTVM:DOUBLE
p 2144 2600 100 0 0 FTVN:DOUBLE
p 2144 2568 100 0 0 FTVO:DOUBLE
p 2144 2536 100 0 0 FTVP:DOUBLE
p 2144 2504 100 0 0 FTVQ:DOUBLE
p 2144 2472 100 0 0 FTVR:DOUBLE
p 2144 2440 100 0 0 FTVS:DOUBLE
p 2144 2408 100 0 0 FTVT:DOUBLE
p 2048 992 100 0 1 INAM:dummyInit
p 2080 1856 100 0 0 PINI:YES
p 2144 3112 100 0 0 PREC:4
p 2048 896 100 0 1 SCAN:Passive
p 2048 944 100 0 1 SNAM:CADcontroller
p 2048 1056 100 0 1 name:$(top)setController
[comments]
