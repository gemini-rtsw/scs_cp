[schematic2]
uniq 198
[tools]
[detail]
w 836 1763 100 0 n#195 ecad20.move.SPLK 544 1296 832 1296 832 2240 1024 2240 cadplus.moveP.SPIN
w 1122 2379 100 0 n#194 eseqs.moveSeq.LNK2 1280 2816 1408 2816 1408 2368 896 2368 896 2272 1024 2272 cadplus.moveP.STIN
w 676 2579 100 0 n#193 ecad20.move.VAL 544 2960 672 2960 672 2208 1024 2208 cadplus.moveP.STAT
w 788 1923 100 0 n#192 ecad20.move.STLK 544 1328 784 1328 784 2528 960 2528 eseqs.moveSeq.SLNK
w 1394 2859 100 0 n#191 eseqs.moveSeq.LNK1 1280 2848 1568 2848 ecars.moveC.IVAL
w 930 2859 100 0 n#183 eseqs.moveSeq.DOL1 960 2848 960 2848 hwin.hwin#185.in
w 2482 2443 100 0 n#182 eseqs.stopSeq.LNK1 2416 2432 2608 2432 ecars.stopC.IVAL
w 2418 2379 100 0 n#182 eseqs.stopSeq.LNK3 2416 2368 2480 2368 2480 2432 junction
w 2066 2379 100 0 n#181 hwin.hwin#180.in 2096 2368 2096 2368 eseqs.stopSeq.DOL3
w 2004 2027 100 0 n#179 eseqs.stopSeq.SLNK 2096 2112 2000 2112 2000 1952 1840 1952 ecad2.stop.STLK
w 2516 2171 100 0 n#178 eseqs.stopSeq.LNK2 2416 2400 2512 2400 2512 1952 2672 1952 cadplus.stopP.STIN
w 2242 1899 100 0 n#177 ecad2.stop.VAL 1840 2432 1872 2432 1872 1888 2672 1888 cadplus.stopP.STAT
w 2226 1931 100 0 n#176 ecad2.stop.SPLK 1840 1920 2672 1920 cadplus.stopP.SPIN
w 2066 2443 100 0 n#174 eseqs.stopSeq.DOL1 2096 2432 2096 2432 hwin.hwin#173.in
w 2242 1163 100 0 n#171 ecad2.park.VAL 1840 1696 1872 1696 1872 1152 2672 1152 cadplus.parkP.STAT
w 2066 1707 100 0 n#170 hwin.hwin#169.in 2096 1696 2096 1696 eseqs.parkSeq.DOL1
w 2516 1435 100 0 n#167 eseqs.parkSeq.LNK2 2416 1664 2512 1664 2512 1216 2672 1216 cadplus.parkP.STIN
w 2226 1195 100 0 n#166 ecad2.park.SPLK 1840 1184 2672 1184 cadplus.parkP.SPIN
w 2482 1707 100 0 n#165 eseqs.parkSeq.LNK1 2416 1696 2608 1696 ecars.parkC.IVAL
w 1890 1227 100 0 n#164 ecad2.park.STLK 1840 1216 2000 1216 2000 1376 2096 1376 eseqs.parkSeq.SLNK
w 2898 1483 100 0 n#160 hwout.hwout#159.outp 2928 1472 2928 1472 ecars.parkC.FLNK
w 2898 2219 100 0 n#158 hwout.hwout#157.outp 2928 2208 2928 2208 ecars.stopC.FLNK
w 1858 2635 100 0 n#156 hwout.hwout#155.outp 1888 2624 1888 2624 ecars.moveC.FLNK
s 2816 752 100 0 25-Jun-98
s 2544 720 100 0 checked: S.Prior
s 2544 752 100 0 author: S.Prior
s 3152 736 100 0 1
s 3056 736 100 0 1
s 2784 800 100 0 Move Commands
s 2784 864 100 0 Secondary Control System
s 2592 3024 100 0 move.sch
[cell use]
use periscope 1648 727 100 0 periscope#197
xform 0 1824 928
use hwin 1904 2391 100 0 hwin#173
xform 0 2000 2432
p 1907 2424 100 0 -1 val(in):2
use hwin 1904 1655 100 0 hwin#169
xform 0 2000 1696
p 1907 1688 100 0 -1 val(in):2
use hwin 1904 2327 100 0 hwin#180
xform 0 2000 2368
p 1907 2360 100 0 -1 val(in):0
use hwin 768 2807 100 0 hwin#185
xform 0 864 2848
p 771 2840 100 0 -1 val(in):2
use eseqs 2096 2023 100 0 stopSeq
xform 0 2256 2272
p 2176 1920 100 0 0 DLY3:2
p 2096 1968 100 0 1 PV:$(top)
p 2432 2432 75 1024 -1 pproc(LNK1):PP
p 2432 2368 75 1024 -1 pproc(LNK3):PP
use eseqs 2096 1287 100 0 parkSeq
xform 0 2256 1536
p 2096 1232 100 0 1 PV:$(top)
p 2432 1696 75 1024 -1 pproc(LNK1):PP
use eseqs 960 2439 100 0 moveSeq
xform 0 1120 2688
p 1040 2336 100 0 0 DLY3:2
p 960 2384 100 0 1 PV:$(top)
p 1296 2848 75 1024 -1 pproc(LNK1):PP
p 1296 2784 75 1024 -1 pproc(LNK3):NPP
use baffle 672 727 100 0 baffle#162
xform 0 832 936
use follow 1152 727 100 0 follow#161
xform 0 1312 936
use hwout 1888 2583 100 0 hwout#155
xform 0 1984 2624
p 1984 2615 100 0 -1 val(outp):$(top)allCar.VAL
use hwout 2928 2167 100 0 hwout#157
xform 0 3024 2208
p 3024 2199 100 0 -1 val(outp):$(top)allCar.VAL
use hwout 2928 1431 100 0 hwout#159
xform 0 3024 1472
p 2944 1424 100 0 -1 val(outp):$(top)allCar.VAL
use coords 160 727 100 0 coords#154
xform 0 328 936
p 160 752 100 0 1 seta:group config:
use ecad20 224 1239 100 0 move
xform 0 384 2128
p 320 2864 100 0 0 DESC:move CAD record
p 320 2704 100 0 0 FTVA:DOUBLE
p 320 2672 100 0 0 FTVB:DOUBLE
p 320 2640 100 0 0 FTVC:DOUBLE
p 320 2608 100 0 0 FTVD:DOUBLE
p 320 2576 100 0 0 FTVE:DOUBLE
p 320 2544 100 0 0 FTVF:DOUBLE
p 320 2512 100 0 0 FTVG:DOUBLE
p 320 2480 100 0 0 FTVH:DOUBLE
p 320 2448 100 0 0 FTVI:DOUBLE
p 320 1888 100 0 1 INAM:
p 320 1968 100 0 0 PREC:4
p 244 380 100 0 1 PV:$(top)
p 320 1840 100 0 1 SNAM:CADmove
use ecars 1568 2567 100 0 moveC
xform 0 1728 2736
p 1568 2528 100 0 1 PV:$(top)
use ecars 2608 1415 100 0 parkC
xform 0 2768 1584
p 2608 1376 100 0 1 PV:$(top)
p 2704 1776 100 0 0 name:$(top)$(I)
use ecars 2608 2151 100 0 stopC
xform 0 2768 2320
p 2608 2112 100 0 1 PV:$(top)
p 2688 2512 100 0 0 name:$(top)$(I)
use cadplus 1024 2119 100 0 moveP
xform 0 1088 2224
p 1024 2096 100 0 1 seta:plus $(top)moveP:
use cadplus 2672 1063 100 0 parkP
xform 0 2736 1168
p 2672 1040 100 0 1 seta:plus $(top)parkP:
use cadplus 2672 1799 100 0 stopP
xform 0 2736 1904
p 2672 1776 100 0 1 seta:plus $(top)stopP:
use ecad2 1520 1127 100 0 park
xform 0 1680 1440
p 1584 2184 100 0 0 DESC:Park
p 1520 1088 100 0 1 PV:$(top)
p 1584 2216 100 0 0 SNAM:CADpark
p 1600 1760 100 0 0 name:$(top)$(I)
use ecad2 1520 1863 100 0 stop
xform 0 1680 2176
p 1584 2920 100 0 0 DESC:stopm2 command
p 1520 1792 100 0 1 PV:$(top)
p 1584 2952 100 0 0 SNAM:CADstop
p 1632 2480 100 0 0 name:$(top)$(I)
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
