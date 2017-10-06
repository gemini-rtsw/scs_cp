[schematic2]
uniq 122
[tools]
[detail]
w 1572 2363 100 0 n#121 eseqs.moveActuatorSeq.LNK2 1440 2784 1568 2784 1568 1952 2048 1952 cadplus.actsP.STIN
w 996 2219 100 0 n#120 ecad8.moveActuator.STLK 656 1952 992 1952 992 2496 1120 2496 eseqs.moveActuatorSeq.SLNK
w 1090 2827 100 0 n#119 hwin.hwin#118.in 1120 2816 1120 2816 eseqs.moveActuatorSeq.DOL1
w 1394 1899 100 0 n#113 ecad8.moveActuator.VAL 656 2816 800 2816 800 1888 2048 1888 cadplus.actsP.STAT
w 1858 2827 100 0 n#112 eseqs.moveActuatorSeq.LNK1 1440 2816 2336 2816 ecars.moveActuatorC.IVAL
w 1322 1931 100 0 n#107 ecad8.moveActuator.SPLK 656 1920 2048 1920 cadplus.actsP.SPIN
w 2626 2603 100 0 n#106 hwout.hwout#105.outp 2656 2592 2656 2592 ecars.moveActuatorC.FLNK
s 2800 800 100 0 coordinate systems
s 2800 816 100 0 move commands using different
s 2816 752 100 0 25-Jun-98
s 2544 720 100 0 checked: S.Prior
s 2544 752 100 0 author: S.Prior
s 3152 736 100 0 1
s 3056 736 100 0 1
s 2784 864 100 0 Secondary Control System
s 2592 3024 100 0 coords.sch
[cell use]
use hwin 928 2775 100 0 hwin#118
xform 0 1024 2816
p 931 2808 100 0 -1 val(in):2
use eseqs 1120 2407 100 0 moveActuatorSeq
xform 0 1280 2656
p 1168 2336 100 0 1 PV:$(top)
p 1456 2816 75 1024 -1 pproc(LNK1):PP
p 1456 2784 75 1024 -1 pproc(LNK2):NPP
p 1456 2752 75 1024 -1 pproc(LNK3):NPP
p 1456 2720 75 1024 -1 pproc(LNK4):NPP
use hwout 2656 2551 100 0 hwout#105
xform 0 2752 2592
p 2752 2583 100 0 -1 val(outp):$(top)allCar.VAL
use ecad8 336 1863 100 0 moveActuator
xform 0 496 2368
p 432 3016 100 0 0 FTVA:DOUBLE
p 432 2984 100 0 0 FTVB:DOUBLE
p 432 2952 100 0 0 FTVC:DOUBLE
p 432 2920 100 0 0 FTVD:DOUBLE
p 432 2888 100 0 0 FTVE:DOUBLE
p 432 2856 100 0 0 FTVF:DOUBLE
p 432 3112 100 0 0 PREC:4
p 356 1036 100 0 0 PV:$(top)
p 336 1824 100 0 1 SNAM:CADactuators
use ecars 2336 2535 100 0 moveActuatorC
xform 0 2496 2704
p 2324 2284 100 0 0 PV:$(top)
use cadplus 2048 1799 100 0 actsP
xform 0 2112 1904
p 2048 1776 100 0 1 seta:plus $(top)moveActuatorP:
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
