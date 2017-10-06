[schematic2]
uniq 46
[tools]
[detail]
w 280 2075 100 0 n#38 junction 272 2064 336 2064 ebos.paxisCtrl.DOL
w 836 1899 100 0 n#38 inhier.axisIn.P 192 2080 272 2080 272 1888 1472 1888 1472 1664 1712 1664 ebos.paxisCtrl2.DOL
w 1096 2043 100 0 n#45 estringouts.paxisCtrlStr.OUT 1072 2032 1168 2032 1168 2160 1232 2160 ecad2.phasorControl.B
w 728 2091 100 0 n#44 ebos.paxisCtrl.VAL 592 2032 688 2032 688 2080 816 2080 estringouts.paxisCtrlStr.DOL
w 632 2075 100 0 n#43 ebos.paxisCtrl.FLNK 592 2064 720 2064 720 2048 816 2048 estringouts.paxisCtrlStr.SLNK
w 2040 1403 100 0 n#34 ebos.paxisCtrl2.OUT 1968 1600 2016 1600 2016 1392 2112 1392 ecad20.setPhasorControl.O
w 1144 2235 100 0 n#27 estringouts.phasorOnStr.OUT 1104 2240 1104 2224 1232 2224 ecad2.phasorControl.A
w 768 2267 100 0 n#26 ebos.phasorOn.FLNK 640 2336 736 2336 736 2256 848 2256 estringouts.phasorOnStr.SLNK
w 688 2315 100 0 n#25 ebos.phasorOn.VAL 640 2304 784 2304 784 2288 848 2288 estringouts.phasorOnStr.DOL
w 432 1707 100 0 n#24 hwin.hwin#21.in 368 1696 544 1696 eseqs.phasorControlSeq.DOL2
w 432 1739 100 0 n#23 hwin.hwin#22.in 368 1728 544 1728 eseqs.phasorControlSeq.DOL1
w 1000 1851 100 0 n#20 ecad2.phasorControl.STLK 1552 1936 1648 1936 1648 1840 400 1840 400 1408 544 1408 eseqs.phasorControlSeq.SLNK
w 1352 1515 100 0 n#19 ecars.phasorControlC.FLNK 1344 1504 1408 1504 hwout.hwout#18.outp
w 880 1707 100 0 n#17 eseqs.phasorControlSeq.LNK2 864 1696 944 1696 944 1728 junction
w 920 1739 100 0 n#17 eseqs.phasorControlSeq.LNK1 864 1728 1024 1728 ecars.phasorControlC.IVAL
w 3064 2267 100 0 n#10 ecars.setPhasorControlC.FLNK 3072 2256 3104 2256 hwout.hwout#8.outp
w 3150 1179 100 0 n#9 eseqs.setPhasorControlSeq.LNK1 3104 1168 3232 1168 junction
w 2926 1355 100 0 n#9 eseqs.setPhasorControlSeq.LNK2 3104 1136 3232 1136 3232 1344 2656 1344 2656 2480 2752 2480 ecars.setPhasorControlC.IVAL
w 2726 1147 100 0 n#6 hwin.hwin#4.in 2704 1136 2784 1136 eseqs.setPhasorControlSeq.DOL2
w 2726 1179 100 0 n#5 hwin.hwin#3.in 2704 1168 2784 1168 eseqs.setPhasorControlSeq.DOL1
w 2590 859 100 0 n#2 ecad20.setPhasorControl.STLK 2432 848 2784 848 eseqs.setPhasorControlSeq.SLNK
s 1824 2288 100 0 phasor->frequency
s 1824 2224 100 0 phasor->amplitude
s 1776 2160 100 0 phasor->sampleRate
[cell use]
use estringouts 816 1975 100 0 paxisCtrlStr
xform 0 944 2048
p 816 1936 100 0 1 OMSL:closed_loop
p 832 2128 100 0 1 PV:$(top)$(axis)
use estringouts 848 2183 100 0 phasorOnStr
xform 0 976 2256
p 848 2144 100 0 1 OMSL:closed_loop
p 864 2336 100 0 1 PV:$(top)$(axis)
use ebos 336 1943 100 0 paxisCtrl
xform 0 464 2032
p 528 1920 100 0 1 ONAM:YTILT
p 416 2000 100 0 1 PINI:YES
p 352 2144 100 0 1 PV:$(top)$(axis)
p 528 1952 100 0 1 ZNAM:XTILT
use ebos 384 2215 100 0 phasorOn
xform 0 512 2304
p 496 2176 100 0 1 ONAM:ON
p 400 2416 100 0 1 PV:$(top)$(axis)
p 496 2208 100 0 1 ZNAM:OFF
use ebos 1712 1543 100 0 paxisCtrl2
xform 0 1840 1632
p 1728 1456 100 0 1 ONAM:ytilt
p 1856 1536 100 0 1 PINI:YES
p 1728 1744 100 0 1 PV:$(top)$(axis)
p 1728 1488 100 0 1 ZNAM:xtilt
use inhier 176 2039 100 0 axisIn
xform 0 192 2080
use hwin 176 1687 100 0 hwin#22
xform 0 272 1728
p 176 1728 100 0 -1 val(in):$(CAR_BUSY)
use hwin 176 1655 100 0 hwin#21
xform 0 272 1696
p 176 1696 100 0 -1 val(in):$(CAR_IDLE)
use hwin 2512 1095 100 0 hwin#4
xform 0 2608 1136
p 2512 1136 100 0 -1 val(in):$(CAR_IDLE)
use hwin 2512 1127 100 0 hwin#3
xform 0 2608 1168
p 2512 1168 100 0 -1 val(in):$(CAR_BUSY)
use hwout 1408 1463 100 0 hwout#18
xform 0 1504 1504
p 1504 1495 100 0 -1 val(outp):$(top)allCar.VAL
use hwout 3104 2215 100 0 hwout#8
xform 0 3200 2256
p 3104 2208 100 0 -1 val(outp):$(top)allCar.VAL
use ecars 1024 1447 100 0 phasorControlC
xform 0 1184 1616
p 1088 1776 100 0 1 PV:$(top)$(axis)
use ecars 2752 2199 100 0 setPhasorControlC
xform 0 2912 2368
p 2848 2192 100 0 0 PRIO:LOW
p 2816 2544 100 0 1 PV:$(top)$(axis)
p 2864 2144 100 1024 -1 name:$(top)$(I)
use eseqs 544 1319 100 0 phasorControlSeq
xform 0 704 1568
p 624 1520 100 0 1 DLY2:1.0e+00
p 160 1726 100 0 0 PINI:NO
p 592 1792 100 0 1 PV:$(top)$(axis)
p 512 1728 75 1280 -1 pproc(DOL1):NPP
p 880 1728 75 1024 -1 pproc(LNK1):PP
p 880 1696 75 1024 -1 pproc(LNK2):PP
use eseqs 2784 759 100 0 setPhasorControlSeq
xform 0 2944 1008
p 2848 1264 100 0 1 PV:$(top)$(axis)
p 3120 1168 75 1024 -1 pproc(LNK1):PP
p 3120 1136 75 1024 -1 pproc(LNK2):PP
use ecad2 1232 1847 100 0 phasorControl
xform 0 1392 2160
p 1328 2160 100 0 1 FTVA:LONG
p 1328 2128 100 0 1 FTVB:LONG
p 1296 2464 100 0 1 PV:$(top)$(axis)
p 1248 2064 100 0 1 SNAM:CADphasorControl
use ecad20 2112 759 100 0 setPhasorControl
xform 0 2272 1648
p 2208 2288 100 0 1 FTVA:DOUBLE
p 2208 2192 100 0 1 FTVB:DOUBLE
p 2208 2160 100 0 1 FTVC:DOUBLE
p 2208 2128 100 0 1 FTVD:DOUBLE
p 2208 2096 100 0 1 FTVE:DOUBLE
p 2208 2064 100 0 1 FTVF:DOUBLE
p 2208 2032 100 0 1 FTVG:DOUBLE
p 2208 2000 100 0 1 FTVH:DOUBLE
p 2208 1968 100 0 1 FTVI:DOUBLE
p 2208 1936 100 0 1 FTVJ:DOUBLE
p 2208 1904 100 0 1 FTVK:DOUBLE
p 2208 1872 100 0 1 FTVL:DOUBLE
p 2208 1840 100 0 1 FTVM:DOUBLE
p 2208 1808 100 0 1 FTVN:DOUBLE
p 2208 1776 100 0 1 FTVO:DOUBLE
p 2208 1744 100 0 1 FTVP:DOUBLE
p 2208 1712 100 0 1 FTVQ:DOUBLE
p 2208 1680 100 0 1 FTVR:DOUBLE
p 2208 1648 100 0 1 FTVS:DOUBLE
p 2208 1616 100 0 1 FTVT:DOUBLE
p 2128 624 100 0 1 INAM:dummyInit
p 2208 1488 100 0 0 PREC:4
p 2208 1456 100 0 0 PRIO:LOW
p 2176 2544 100 0 1 PV:$(top)$(axis)
p 2128 512 100 0 1 SCAN:Passive
p 2128 576 100 0 1 SNAM:CADsetPhasorControl
p 2240 688 100 1024 1 name:$(top)$(I)
p 2080 2256 75 1280 -1 pproc(INPA):NPP
use bc200tr 32 216 -100 0 frame
xform 0 1712 1520
[comments]
