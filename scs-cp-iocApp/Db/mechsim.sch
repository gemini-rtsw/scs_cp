[schematic2]
uniq 515
[tools]
[detail]
w 290 1131 100 0 n#514 simaxis.simaxis#501.flnk -192 1120 832 1120 outhier.FLNK.p
w 290 1387 100 0 n#513 simaxis.simaxis#501.coincidence -192 1376 832 1376 outhier.inposition.p
w 290 1451 100 0 n#512 simaxis.simaxis#501.position -192 1440 832 1440 outhier.position.p
w -782 1419 100 0 n#511 eais.b.VAL -1200 1248 -928 1248 -928 1408 -576 1408 simaxis.simaxis#501.B
w -766 1387 100 0 n#510 simaxis.simaxis#501.C -576 1376 -896 1376 -896 1056 -1200 1056 eais.c.VAL
w -860 1099 100 0 n#509 eais.custom.VAL -1200 864 -864 864 -864 1344 -576 1344 simaxis.simaxis#501.custom
w -828 987 100 0 n#508 eais.guide.VAL -1200 672 -832 672 -832 1312 -576 1312 simaxis.simaxis#501.guide
w -796 843 100 0 n#507 simaxis.simaxis#501.beam -576 1280 -800 1280 -800 416 -1472 416 inhier.select.P
w -764 747 100 0 n#506 inhier.enable.P -1472 256 -768 256 -768 1248 -576 1248 simaxis.simaxis#501.enable
w -732 571 100 0 n#504 eais.window.VAL -1184 -64 -736 -64 -736 1216 -576 1216 simaxis.simaxis#501.tolerance
w -700 379 100 0 n#503 inhier.SLNK.P -1472 -352 -704 -352 -704 1120 -576 1120 simaxis.simaxis#501.slnk
w -918 1451 100 0 n#502 eais.a.VAL -1200 1440 -576 1440 simaxis.simaxis#501.A
s 608 1664 100 0 mechanism.sch
s 784 -496 100 0 Secondary Control System
s 784 -560 100 0 Mechanism Schematic
s 560 -608 100 0 author: S.Prior
s 560 -640 100 0 checked: S. Prior
s 816 -624 100 0 26-AUG-96
s 1072 -624 100 0 1
s 1168 -624 100 0 1
[cell use]
use simaxis -576 1015 100 0 simaxis#501
xform 0 -384 1256
use inhier -1488 -393 100 0 SLNK
xform 0 -1472 -352
use inhier -1488 215 100 0 enable
xform 0 -1472 256
use inhier -1488 375 100 0 select
xform 0 -1472 416
use eais -1440 -121 100 0 window
xform 0 -1312 -48
p -1651 25 100 0 0 DESC:in position window size
p -1696 -306 100 0 0 EGU:none
p -1696 -210 100 0 0 PREC:2
p -1344 -128 100 0 1 PV:$(top)$(axis)
use eais -1456 615 100 0 guide
xform 0 -1328 688
p -1667 761 100 0 0 DESC:guide correction value
p -1712 526 100 0 0 PREC:5
p -1360 608 100 0 1 PV:$(top)$(axis)
p -1344 752 100 1024 -1 name:$(top)guide
use eais -1456 1383 100 0 a
xform 0 -1328 1456
p -1667 1529 100 0 0 DESC:beam a demand
p -1712 1166 100 0 0 HOPR:100
p -1712 1134 100 0 0 LOPR:-100
p -1712 1294 100 0 0 PREC:2
p -1392 1376 100 0 1 PV:$(top)$(axis)
p -1344 1520 100 1024 -1 name:$(top)a
use eais -1456 1191 100 0 b
xform 0 -1328 1264
p -1667 1337 100 0 0 DESC:beam b demand
p -1712 1102 100 0 0 PREC:2
p -1392 1184 100 0 1 PV:$(top)$(axis)
p -1344 1328 100 1024 -1 name:$(top)b
use eais -1456 999 100 0 c
xform 0 -1328 1072
p -1667 1145 100 0 0 DESC:beam c demand
p -1712 910 100 0 0 PREC:2
p -1392 992 100 0 1 PV:$(top)$(axis)
p -1344 1136 100 1024 -1 name:$(top)c
use eais -1456 807 100 0 custom
xform 0 -1328 880
p -1667 953 100 0 0 DESC:custom demand
p -1712 718 100 0 0 PREC:2
p -1360 800 100 0 1 PV:$(top)$(axis)
p -1328 944 100 1024 -1 name:$(top)custom
use outhier 800 1079 100 0 FLNK
xform 0 816 1120
use outhier 800 1335 100 0 inposition
xform 0 816 1376
use outhier 800 1399 100 0 position
xform 0 816 1440
use bc200tr -2016 -776 -100 0 frame
xform 0 -336 528
[comments]
