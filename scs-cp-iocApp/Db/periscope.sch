[schematic2]
uniq 210
[tools]
[detail]
w 378 2075 100 0 n#209 ebos.periscope.FLNK 368 2064 448 2064 448 2048 528 2048 528 2032 576 2032 estringouts.centralStr.SLNK
w 2226 2091 100 0 n#195 eseqs.periscopeSeq.LNK2 2176 2080 2336 2080 2336 2112 junction
w 2362 2123 100 0 n#195 eseqs.periscopeSeq.LNK1 2176 2112 2608 2112 ecars.movePeriscopeC.IVAL
w 1860 2075 100 2 n#208 hwin.hwin#207.in 1856 2080 1856 2080 eseqs.periscopeSeq.DOL2
w 986 2027 100 0 n#206 estringouts.centralStr.OUT 832 2016 1200 2016 ecad2.move.A
w 394 2034 100 0 n#204 ebos.periscope.VAL 368 2032 480 2032 480 2064 576 2064 estringouts.centralStr.DOL
w 1826 2123 100 0 n#198 hwin.hwin#197.in 1856 2112 1856 2112 eseqs.periscopeSeq.DOL1
w 1586 1739 100 0 n#192 ecad2.move.STLK 1520 1728 1712 1728 1712 1792 1856 1792 eseqs.periscopeSeq.SLNK
w 2898 1899 100 0 n#179 hwout.hwout#178.outp 2928 1888 2928 1888 ecars.movePeriscopeC.FLNK
s 2544 3056 100 0 $Id: periscope.sch,v 1.2 2005/07/15 20:15:13 gemvx Exp $
s 2784 864 100 0 Secondary Control System
s 2784 800 100 0 Baffle position demands
s 3056 736 100 0 1
s 3152 736 100 0 1
s 2544 752 100 0 author: S.Prior
s 2544 720 100 0 checked: S.Prior
s 2816 752 100 0 25-Jun-98
[cell use]
use hwin 1664 2039 100 0 hwin#207
xform 0 1760 2080
p 1667 2072 100 0 -1 val(in):$(CAR_IDLE)
use hwin 1664 2071 100 0 hwin#197
xform 0 1760 2112
p 1667 2104 100 0 -1 val(in):$(CAR_BUSY)
use estringouts 600 1960 100 0 centralStr
xform 0 704 2032
p 512 1838 100 0 0 OMSL:closed_loop
p 512 1998 100 0 0 VAL:
use eseqs 1856 1703 100 0 periscopeSeq
xform 0 2016 1952
p 1472 -228 100 0 0 DLY2:2
p 1920 1664 100 0 1 PV:$(top)
p 2192 2112 75 1024 -1 pproc(LNK1):PP
p 2192 2080 75 1024 -1 pproc(LNK2):PP
use hwout 2928 1847 100 0 hwout#178
xform 0 3024 1888
p 3024 1879 100 0 -1 val(outp):$(top)allCar.VAL
use ecad2 1200 1639 100 0 move
xform 0 1360 1952
p 1264 2696 100 0 0 DESC:move periscope commands
p 1264 2440 100 0 0 FTVA:LONG
p 1264 2408 100 0 0 FTVB:LONG
p 1264 2728 100 0 0 SNAM:CADmovePeriscope
p 1280 2288 100 0 -1 name:$(top)movePeriscope
use ecars 2608 1831 100 0 movePeriscopeC
xform 0 2768 2000
p 2608 1792 100 0 1 PV:$(top)
p 2656 2176 100 0 0 name:$(top)$(I)
use ebos 112 1943 100 0 periscope
xform 0 240 2032
p -120 2188 100 0 0 DESC:persicope demand
p -208 1886 100 0 0 ONAM:OPEN
p -208 2110 100 0 0 SCAN:Passive
p -208 1918 100 0 0 ZNAM:CLOSED
p 160 1920 100 0 -1 name:$(top)central
p -96 2190 100 0 0 primitive:ebo
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
