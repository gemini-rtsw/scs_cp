[schematic2]
uniq 101
[tools]
[detail]
w 632 1163 100 0 n#99 ecad20.ecad20#69.STLK 512 1152 800 1152 eseqs.decsConfigSeq.SLNK
w 1144 1451 100 0 n#100 eseqs.decsConfigSeq.LNK2 1120 1440 1216 1440 1216 1472 junction
w 708 2187 100 0 n#100 eseqs.decsConfigSeq.LNK1 1120 1472 1216 1472 1216 1600 704 1600 704 2784 1184 2784 ecars.ecars#71.IVAL
w 776 1483 100 0 n#93 eseqs.decsConfigSeq.DOL1 800 1472 800 1472 hwin.hwin#96.in
w 776 1451 100 0 n#92 eseqs.decsConfigSeq.DOL2 800 1440 800 1440 hwin.hwin#95.in
w 1480 2571 100 0 n#75 hwout.hwout#73.outp 1504 2560 1504 2560 ecars.ecars#71.FLNK
s 2592 3024 100 0 decsEng.sch
s 2816 752 100 0 04-Mar-98
s 2544 720 100 0 checked: S.Prior
s 2544 752 100 0 author: S.Prior
s 3152 736 100 0 1
s 3056 736 100 0 1
s 2784 800 100 0 DECS Engineering Controls
s 2784 864 100 0 Secondary Control System
[cell use]
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
use hwin 608 1399 100 0 hwin#95
xform 0 704 1440
p 611 1432 100 0 -1 val(in):$(CAR_IDLE)
use hwin 608 1431 100 0 hwin#96
xform 0 704 1472
p 611 1464 100 0 -1 val(in):$(CAR_BUSY)
use eseqs 800 1063 100 0 decsConfigSeq
xform 0 960 1312
p 880 992 100 0 0 DLY2:1
p 864 1040 100 0 1 PV:$(top)
p 768 1472 75 1280 -1 pproc(DOL1):NPP
p 768 1440 75 1280 -1 pproc(DOL2):NPP
p 1136 1472 75 1024 -1 pproc(LNK1):PP
p 1136 1440 75 1024 -1 pproc(LNK2):PP
use hwout 1504 2519 100 0 hwout#73
xform 0 1600 2560
p 1536 2496 100 0 -1 val(outp):$(top)allCar.VAL
use ecars 1184 2503 100 0 ecars#71
xform 0 1344 2672
p 1248 2864 100 0 -1 name:$(top)decsConfigC
use ecad20 192 1063 100 0 ecad20#69
xform 0 352 1952
p 352 3240 100 0 0 DESC:decs gain/bin/smoothing
p 352 2984 100 0 0 FTVA:DOUBLE
p 352 2952 100 0 0 FTVB:DOUBLE
p 352 2920 100 0 0 FTVC:DOUBLE
p 352 2888 100 0 0 FTVD:LONG
p 352 2856 100 0 0 FTVE:LONG
p 352 2824 100 0 0 FTVF:LONG
p 352 2792 100 0 0 FTVG:DOUBLE
p 352 2760 100 0 0 FTVH:DOUBLE
p 352 2728 100 0 0 FTVI:DOUBLE
p 240 992 100 0 1 INAM:
p 352 3080 100 0 0 PREC:2
p 240 896 100 0 1 SCAN:Passive
p 240 944 100 0 1 SNAM:CADdecsAdjust
p 240 1040 100 0 -1 name:$(top)decsConfig
[comments]
