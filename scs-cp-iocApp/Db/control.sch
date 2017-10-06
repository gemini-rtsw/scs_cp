[schematic2]
uniq 141
[tools]
[detail]
w 2370 1259 100 0 n#140 hwin.hwin#139.in 2352 1248 2448 1248 phasorControl.phasorControlY.axisIn
w 2370 1675 100 0 n#137 hwin.hwin#138.in 2352 1664 2448 1664 phasorControl.phasorControlX.axisIn
w 2394 2043 100 0 n#134 hwin.hwin#133.in 2400 2032 2448 2032 vtkControl.vtkControlY.axisIn
w 2370 2395 100 0 n#132 hwin.hwin#131.in 2352 2384 2448 2384 vtkControl.vtkControlX.axisIn
w 884 1531 100 0 n#126 ecad8.tolerance.STLK 592 1264 880 1264 880 1808 976 1808 eseqs.toleranceSeq.SLNK
w 692 1659 100 0 n#125 ecad8.tolerance.VAL 592 2128 688 2128 688 1200 1584 1200 cadplus.tolP.STAT
w 1428 1675 100 0 n#123 eseqs.toleranceSeq.LNK2 1296 2096 1424 2096 1424 1264 1584 1264 cadplus.tolP.STIN
w 1058 1243 100 0 n#122 ecad8.tolerance.SPLK 592 1232 1584 1232 cadplus.tolP.SPIN
w 1426 2139 100 0 n#121 junction 1392 2128 1520 2128 ecars.toleranceC.IVAL
w 1314 2075 100 0 n#121 eseqs.toleranceSeq.LNK3 1296 2064 1392 2064 1392 2128 1296 2128 eseqs.toleranceSeq.LNK1
w 946 2075 100 0 n#120 eseqs.toleranceSeq.DOL3 976 2064 976 2064 hwin.hwin#119.in
w 946 2139 100 0 n#118 eseqs.toleranceSeq.DOL1 976 2128 976 2128 hwin.hwin#117.in
w 1250 2763 100 0 n#114 hwin.hwin#113.in 1280 2752 1280 2752 eseqs.servpbwSeq.DOL3
w 1250 2827 100 0 n#112 hwin.hwin#111.in 1280 2816 1280 2816 eseqs.servpbwSeq.DOL1
w 994 2347 100 0 n#110 ecad2.servoBandwidth.STLK 896 2336 1152 2336 1152 2496 1280 2496 eseqs.servpbwSeq.SLNK
w 1410 2283 100 0 n#109 ecad2.servoBandwidth.VAL 896 2816 992 2816 992 2272 1888 2272 cadplus.servoP.STAT
w 1732 2555 100 0 n#108 eseqs.servpbwSeq.LNK2 1600 2784 1728 2784 1728 2336 1888 2336 cadplus.servoP.STIN
w 1362 2315 100 0 n#107 ecad2.servoBandwidth.SPLK 896 2304 1888 2304 cadplus.servoP.SPIN
w 1618 2763 100 0 n#106 eseqs.servpbwSeq.LNK3 1600 2752 1696 2752 1696 2816 junction
w 1682 2827 100 0 n#106 eseqs.servpbwSeq.LNK1 1600 2816 1824 2816 ecars.servoBandwidthC.IVAL
w 1816 1915 100 0 n#94 hwout.hwout#93.outp 1840 1904 1840 1904 ecars.toleranceC.FLNK
w 2120 2603 100 0 n#92 hwout.hwout#91.outp 2144 2592 2144 2592 ecars.servoBandwidthC.FLNK
s 2816 752 100 0 29-Jun-98
s 2544 720 100 0 checked: S.Prior
s 2544 752 100 0 author: S.Prior
s 3152 736 100 0 1
s 3056 736 100 0 1
s 2784 800 100 0 Configuration Commands
s 2784 864 100 0 Secondary Control System
s 2592 3024 100 0 control.sch
[cell use]
use hwin 2160 1207 100 0 hwin#139
xform 0 2256 1248
p 2163 1240 100 0 -1 val(in):$(YTILT)
use hwin 2160 1623 100 0 hwin#138
xform 0 2256 1664
p 2163 1656 100 0 -1 val(in):$(XTILT)
use phasorControl 2448 1063 100 0 phasorControlY
xform 0 2800 1232
p 2576 1328 100 0 1 seta:axis Y:
use phasorControl 2448 1479 100 0 phasorControlX
xform 0 2800 1648
p 2576 1744 100 0 1 seta:axis X:
use hwin 784 2023 100 0 hwin#119
xform 0 880 2064
p 787 2056 100 0 -1 val(in):0
use hwin 784 2087 100 0 hwin#117
xform 0 880 2128
p 787 2120 100 0 -1 val(in):2
use hwin 1088 2711 100 0 hwin#113
xform 0 1184 2752
p 1091 2744 100 0 -1 val(in):0
use hwin 1088 2775 100 0 hwin#111
xform 0 1184 2816
p 1091 2808 100 0 -1 val(in):2
use hwin 2160 2343 100 0 hwin#131
xform 0 2256 2384
p 2163 2376 100 0 -1 val(in):$(XTILT)
use hwin 2208 1991 100 0 hwin#133
xform 0 2304 2032
p 2211 2024 100 0 -1 val(in):$(YTILT)
use vtkControl 2448 2199 100 0 vtkControlX
xform 0 2800 2368
p 2576 2480 100 0 1 seta:axis X:
use vtkControl 2448 1847 100 0 vtkControlY
xform 0 2800 2016
p 2560 2128 100 0 1 seta:axis Y:
use pid 2832 2583 100 0 pid#128
xform 0 2992 2784
use beamJog 2400 2583 100 0 beamJog#127
xform 0 2560 2784
use eseqs 976 1719 100 0 toleranceSeq
xform 0 1136 1968
p 1056 1616 100 0 0 DLY3:2
p 1040 1664 100 0 1 PV:$(top)
p 1312 2128 75 1024 -1 pproc(LNK1):PP
p 1312 2064 75 1024 -1 pproc(LNK3):PP
use eseqs 1280 2407 100 0 servpbwSeq
xform 0 1440 2656
p 1360 2304 100 0 0 DLY3:2
p 1344 2352 100 0 1 PV:$(top)
p 1616 2816 75 1024 -1 pproc(LNK1):PP
p 1616 2752 75 1024 -1 pproc(LNK3):PP
use frame 1616 679 100 0 frame#98
xform 0 1776 888
use decsEng 2080 695 100 0 decsEng#97
xform 0 2240 896
use hwout 2144 2551 100 0 hwout#91
xform 0 2240 2592
p 2128 2544 100 0 -1 val(outp):$(top)allCar.VAL
use hwout 1840 1863 100 0 hwout#93
xform 0 1936 1904
p 1824 1856 100 0 -1 val(outp):$(top)allCar.VAL
use ecad2 576 2247 100 0 servoBandwidth
xform 0 736 2560
p 640 3304 100 0 0 DESC:servo bandwidth command
p 640 3048 100 0 0 FTVA:DOUBLE
p 640 2192 100 0 1 PV:$(top)
p 640 3336 100 0 0 SNAM:CADservoBandwidth
p 608 2880 100 0 0 name:$(top)$(I)
use ecad8 272 1175 100 0 tolerance
xform 0 432 1680
p 368 2584 100 0 0 DESC:tolerance values
p 48 1976 100 0 1 FTVA:DOUBLE
p 48 1944 100 0 1 FTVB:DOUBLE
p 48 1912 100 0 1 FTVC:DOUBLE
p 48 1880 100 0 1 FTVD:DOUBLE
p 48 1848 100 0 1 FTVE:DOUBLE
p 48 1808 100 0 1 FTVF:DOUBLE
p 368 1440 100 0 1 PINI:YES
p 368 1520 100 0 0 PREC:2
p 336 1120 100 0 1 PV:$(top)
p 368 1120 100 0 1 SNAM:CADtolerance
p 336 2192 100 0 0 name:$(top)$(I)
use ecars 1520 1847 100 0 toleranceC
xform 0 1680 2016
p 1584 1808 100 0 1 PV:$(top)
p 1584 2208 100 0 0 name:$(top)$(I)
use ecars 1824 2535 100 0 servoBandwidthC
xform 0 1984 2704
p 1888 2464 100 0 1 PV:$(top)
p 1872 2896 100 0 0 name:$(top)$(I)
use cadplus 1584 1111 100 0 tolP
xform 0 1648 1216
p 1584 1088 100 0 1 seta:plus $(top)toleranceP:
use cadplus 1888 2183 100 0 servoP
xform 0 1952 2288
p 1888 2160 100 0 1 seta:plus $(top)servoP:
use bc200tr -96 552 -100 0 frame
xform 0 1584 1856
[comments]
