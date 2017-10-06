[schematic2]
uniq 328
[tools]
[detail]
w 738 1099 100 0 n#327 eaos.tiltStepLimit.VAL 640 1088 896 1088 896 1216 1152 1216 egenSub.followA.INPI
w 738 1451 100 0 n#326 eaos.zStepLimit.VAL 640 1440 896 1440 896 1280 1152 1280 egenSub.followA.INPH
w 1682 1314 100 0 n#322 eaos.eaos#168.FLNK 1808 1056 1888 1056 1888 1312 1536 1312 1536 1536 1632 1536 eaos.trackId.SLNK
w 1506 1570 100 0 n#321 egenSub.followA.VALD 1440 1568 1632 1568 eaos.trackId.DOL
w 1106 2283 100 0 n#319 cadplus.followP.STAT 1600 2272 672 2272 672 2816 576 2816 ecad2.follow.VAL
w 1412 2555 100 0 n#318 eseqs.followSeq.LNK2 1312 2784 1408 2784 1408 2336 1600 2336 cadplus.followP.STIN
w 1058 2315 100 0 n#317 ecad2.follow.SPLK 576 2304 1600 2304 cadplus.followP.SPIN
w 674 2347 100 0 n#316 ecad2.follow.STLK 576 2336 832 2336 832 2496 992 2496 eseqs.followSeq.SLNK
w 1394 2827 100 0 n#315 eseqs.followSeq.LNK1 1312 2816 1536 2816 ecars.followC.IVAL
w 962 2827 100 0 n#307 eseqs.followSeq.DOL1 992 2816 992 2816 hwin.hwin#314.in
w 1410 1675 100 0 n#250 hwout.hwout#249.outp 1440 1664 1440 1664 egenSub.followA.OUTB
w 1466 1035 100 0 n#245 egenSub.followA.FLNK 1440 1024 1552 1024 eaos.eaos#168.SLNK
w 1826 2595 100 0 n#240 hwout.hwout#239.outp 1856 2592 1856 2592 ecars.followC.FLNK
s 2784 864 100 0 Secondary Control System
s 2784 800 100 0 Follow Commands
s 3056 736 100 0 1
s 3152 736 100 0 1
s 2544 752 100 0 author: S.Prior
s 2544 720 100 0 checked: S.Prior
s 2816 752 100 0 25-Jun-98
s 2544 3056 100 0 $Id: follow.sch,v 1.9 2000/01/21 01:36:53 dayle Exp $
[cell use]
use eaos 384 999 100 0 tiltStepLimit
xform 0 512 1088
p 432 1184 100 0 -1 DESC:Tilt step limit
p 576 992 100 0 1 EGU:arcsec
use eaos 384 1351 100 0 zStepLimit
xform 0 512 1440
p 432 1536 100 0 -1 DESC:Focus step limit
p 576 1344 100 0 1 EGU:microns
use eaos 1552 935 100 0 eaos#168
xform 0 1680 1024
p 1296 1006 100 0 0 OMSL:closed_loop
p 1296 878 100 0 0 PREC:2
p 1616 1104 100 0 -1 name:$(top)trigger
use eaos 1656 1448 100 0 trackId
xform 0 1760 1536
p 1412 1722 100 0 0 DESC:Track identifier
p 1600 1262 100 0 0 EGU:none
p 1376 1518 100 0 0 OMSL:closed_loop
use hwin 800 2775 100 0 hwin#314
xform 0 896 2816
p 803 2808 100 0 -1 val(in):2
use eseqs 992 2407 100 0 followSeq
xform 0 1152 2656
p 992 2352 100 0 1 PV:$(top)
p 1328 2816 75 1024 -1 pproc(LNK1):PP
use hwout 1856 2551 100 0 hwout#239
xform 0 1952 2592
p 1904 2544 100 0 -1 val(outp):$(top)allCar.VAL
use hwout 1440 1623 100 0 hwout#249
xform 0 1536 1664
p 1536 1655 100 0 -1 val(outp):$(top)arrayS PP NMS
use egenSub 1152 967 100 0 followA
xform 0 1296 1392
p 995 1707 100 0 0 DESC:Receive demanded positions
p 864 1406 100 0 0 EFLG:ALWAYS
p 929 741 100 0 0 FTA:DOUBLE
p 929 709 100 0 0 FTC:DOUBLE
p 929 741 100 0 0 FTVA:LONG
p 929 741 100 0 0 FTVB:LONG
p 929 677 100 0 0 FTVD:DOUBLE
p 1248 1360 100 0 1 FTVG:LONG
p 1216 864 100 0 1 INAM:initFollowGenSub
p 929 389 100 0 0 NOA:1
p 929 357 100 0 0 NOB:1
p 1184 784 100 0 1 NOJ:14
p 864 1518 100 0 0 PREC:5
p 1232 912 100 0 1 PV:$(top)
p 1216 832 100 0 1 SNAM:receiveTcsDemand
p 976 1728 100 0 0 def(INPA):0.000000000000000e+00
p 816 1152 100 0 0 def(INPJ):0
p 1216 1824 100 0 0 name:$(top)followA
p 960 1744 75 0 1 pproc(INPA):NPP
p 1104 1610 75 0 -1 pproc(INPC):NPP
p 1104 1162 75 0 -1 pproc(INPJ):PP
p 1440 1738 75 0 -1 pproc(OUTA):NPP
p 1440 1674 75 0 -1 pproc(OUTB):NPP
use ebos 2272 2695 100 0 followS
xform 0 2400 2784
p 1952 2638 100 0 0 ONAM:YES
p 2352 2640 100 0 1 PV:$(top)
p 1952 2670 100 0 0 ZNAM:NO
p 2176 2798 100 0 0 ZSV:MAJOR
p 2432 2880 100 1024 1 name:$(top)$(I)
use esirs 2368 1479 100 0 arrayS
xform 0 2576 1632
p 2319 1152 100 0 0 FTVL:LONG
p 2448 1392 100 0 1 PV:$(top)
p 2512 1792 100 0 -1 name:$(top)$(I)
use ecars 1536 2535 100 0 followC
xform 0 1696 2704
p 1616 2464 100 0 1 PV:$(top)
p 1632 2880 100 0 0 name:$(top)$(I)
use cadplus 1600 2183 100 0 followP
xform 0 1664 2288
p 1600 2160 100 0 1 seta:plus $(top)followP:
use ecad2 256 2247 100 0 follow
xform 0 416 2560
p 320 3304 100 0 0 DESC:follow command
p 320 2176 100 0 1 PV:$(top)
p 320 3336 100 0 0 SNAM:CADfollow
p 368 2864 100 0 0 name:$(top)$(I)
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
