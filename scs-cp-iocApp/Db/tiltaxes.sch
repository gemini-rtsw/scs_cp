[schematic2]
uniq 607
[tools]
[detail]
w -1974 1035 100 0 n#606 ebis.enable.VAL -2096 1024 -1792 1024 egenSub.mechanismX.INPA
w -1822 907 100 0 n#590 hwin.hwin#578.in -1792 896 -1792 896 egenSub.mechanismX.INPC
s -496 -704 100 0 Secondary Control System
s -752 -816 100 0 author:S.Prior
s -752 -848 100 0 checked:S.Prior
s -496 -768 100 0 Tilt system simulation axes
s -480 -832 100 0 05-Mar-98
s -240 -816 100 0 1
s -128 -816 100 0 1
s -704 1456 100 0 tiltaxes.sch
s -2384 1328 100 0 for triangular chopping
s -2384 1280 100 0 the demands are calculated
s -2368 1232 100 0 by record driveChop and
s -2384 1184 100 0 read into X and Y tilt
[cell use]
use hwin -1984 855 100 0 hwin#578
xform 0 -1888 896
p -2112 832 100 0 -1 val(in):$(top)driveChop.VALF
use egenSub -1792 263 100 0 mechanismX
xform 0 -1648 688
p -1949 1003 100 0 0 DESC:tilt system axes
p -2015 37 100 0 0 FTA:LONG
p -2015 37 100 0 0 FTB:DOUBLE
p -2015 -27 100 0 0 FTD:DOUBLE
p -2015 -123 100 0 0 FTF:DOUBLE
p -2015 -123 100 0 0 FTG:DOUBLE
p -2015 37 100 0 0 FTVA:LONG
p -2015 37 100 0 0 FTVB:DOUBLE
p -2015 -27 100 0 0 FTVD:DOUBLE
p -1728 176 100 0 1 INAM:
p -2080 814 100 0 0 PREC:2
p -1712 224 100 0 1 PV:$(top)
p -1728 96 100 0 1 SCAN:.01 second
p -1728 144 100 0 1 SNAM:mechSim
use ebis -544 -473 100 0 inpos
xform 0 -416 -400
p -717 -325 100 0 0 DESC:coincidence string
p -544 -560 100 0 1 ONAM:FALSE
p -544 -496 100 0 1 PV:$(top)
p -544 -528 100 0 1 ZNAM:TRUE
use ebis -2352 967 100 0 enable
xform 0 -2224 1040
p -2525 1115 100 0 0 DESC:tilt mechanism enable switch
p -2576 878 100 0 0 ONAM:DISABLED
p -2352 928 100 0 1 PV:$(top)
p -2576 910 100 0 0 ZNAM:ENABLED
use bc200tr -3328 -984 -100 0 frame
xform 0 -1648 320
use elongins -544 -25 100 0 beamselect
xform 0 -416 48
p -755 249 100 0 0 DESC:tilt beam select index
p -544 -48 100 0 1 PV:$(top)
[comments]
