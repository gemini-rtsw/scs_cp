[schematic2]
uniq 209
[tools]
[detail]
w 1890 2811 100 0 n#191 eseqs.driveOffloaderSeq.LNK2 1856 2800 1984 2800 1984 2832 junction
w 1970 2843 100 0 n#191 eseqs.driveOffloaderSeq.LNK1 1856 2832 2144 2832 ecars.driveOffloaderC.IVAL
w 1540 2795 100 2 n#208 hwin.hwin#207.in 1536 2800 1536 2800 eseqs.driveOffloaderSeq.DOL2
w 498 2731 100 0 n#206 embbos.ofldirStr.OUT 416 2720 640 2720 640 2688 800 2688 ecad20.driveOffloader.B
w 1364 1907 100 0 n#192 ecad20.driveOffloader.STLK 1120 1312 1360 1312 1360 2512 1536 2512 eseqs.driveOffloaderSeq.SLNK
w 1506 2843 100 0 n#183 eseqs.driveOffloaderSeq.DOL1 1536 2832 1536 2832 hwin.hwin#185.in
w 2434 2619 100 0 n#156 hwout.hwout#155.outp 2464 2608 2464 2608 ecars.driveOffloaderC.FLNK
s 2816 752 100 0 26-Jan-2000
s 2544 720 100 0 checked: D.Kotturi
s 2544 752 100 0 author: D.Kotturi
s 3152 736 100 0 1
s 3056 736 100 0 1
s 2784 800 100 0 Drive offloader command
s 2784 864 100 0 Secondary Control System
s 2592 3024 100 0 driveOffloader.sch
[cell use]
use hwin 1344 2791 100 0 hwin#185
xform 0 1440 2832
p 1344 2816 100 0 -1 val(in):$(CAR_BUSY)
use hwin 1344 2759 100 0 hwin#207
xform 0 1440 2800
p 1347 2792 100 0 -1 val(in):$(CAR_IDLE)
use embbos 160 2631 100 0 ofldirStr
xform 0 288 2720
p 224 2528 100 0 1 DTYP:Raw Soft Channel
p 224 2560 100 0 1 ONST:IN
p 352 2560 100 0 1 ONVL:1
p 224 2592 100 0 1 ZRST:OUT
p 352 2592 100 0 1 ZRVL:0
use eseqs 1536 2423 100 0 driveOffloaderSeq
xform 0 1696 2672
p 1152 492 100 0 0 DLY2:2
p 1616 2320 100 0 0 DLY3:0.0e+00
p 1536 2368 100 0 1 PV:$(top)
p 1872 2832 75 1024 -1 pproc(LNK1):PP
p 1872 2800 75 1024 -1 pproc(LNK2):PP
p 1872 2768 75 1024 -1 pproc(LNK3):NPP
use hwout 2464 2567 100 0 hwout#155
xform 0 2560 2608
p 2560 2599 100 0 -1 val(outp):$(top)allCar.VAL
use ecad20 800 1223 100 0 driveOffloader
xform 0 960 2112
p 896 2848 100 0 0 DESC:drive offloader CAD record (used for maintenance)
p 896 2688 100 0 0 FTVA:LONG
p 896 2656 100 0 0 FTVB:LONG
p 896 2624 100 0 0 FTVC:LONG
p 896 2592 100 0 0 FTVD:DOUBLE
p 896 2560 100 0 0 FTVE:DOUBLE
p 896 2528 100 0 0 FTVF:DOUBLE
p 896 2496 100 0 0 FTVG:DOUBLE
p 896 2464 100 0 0 FTVH:DOUBLE
p 896 2432 100 0 0 FTVI:DOUBLE
p 896 1952 100 0 0 PREC:4
p 820 364 100 0 1 PV:$(top)
p 896 1824 100 0 0 SNAM:CADdriveOffloader
use ecars 2144 2551 100 0 driveOffloaderC
xform 0 2304 2720
p 2144 2512 100 0 1 PV:$(top)
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
