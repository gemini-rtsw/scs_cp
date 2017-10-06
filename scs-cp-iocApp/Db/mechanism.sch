[schematic2]
uniq 501
[tools]
[detail]
w 804 1355 100 0 n#500 ecalcs.coincidence.FLNK 704 1280 800 1280 800 1440 928 1440 outhier.FLNK.p
w 34 235 100 0 n#498 ecalcs.hardware.FLNK -160 224 288 224 288 1056 416 1056 ecalcs.coincidence.SLNK
w -572 107 100 0 n#496 ecalcs.hardware.INPB -448 352 -576 352 -576 -128 -96 -128 -96 192 -160 192 ecalcs.hardware.VAL
w 368 203 100 0 n#496 junction -96 192 928 192 outhier.position.p
w 60 779 100 0 n#496 ecalcs.coincidence.INPC 416 1376 64 1376 64 192 junction
w -1342 651 100 0 n#494 eais.c.VAL -1440 640 -1184 640 -1184 480 -1024 480 ecalcs.multiplex.INPD
w -1148 667 100 0 n#493 eais.b.VAL -1440 832 -1152 832 -1152 512 -1024 512 ecalcs.multiplex.INPC
w -1116 779 100 0 n#492 eais.a.VAL -1440 1024 -1120 1024 -1120 544 -1024 544 ecalcs.multiplex.INPB
w -1438 683 100 0 n#491 eais.c.FLNK -1440 672 -1376 672 junction
w -1438 875 100 0 n#491 eais.b.FLNK -1440 864 -1376 864 junction
w -1438 1067 100 0 n#491 eais.a.FLNK -1440 1056 -1376 1056 junction
w -1380 715 100 0 n#491 eais.guide.FLNK -1440 1248 -1376 1248 -1376 192 junction
w -1230 203 100 0 n#491 inhier.SLNK.P -1696 192 -1024 192 ecalcs.multiplex.SLNK
w -1438 491 100 0 n#491 eais.custom.FLNK -1440 480 -1376 480 junction
w -1084 891 100 0 n#488 ecalcs.multiplex.INPA -1024 576 -1088 576 -1088 1216 -1440 1216 eais.guide.VAL
w -1486 11 100 0 n#481 inhier.select.P -1696 0 -1216 0 -1216 224 -1024 224 ecalcs.multiplex.INPL
w -1198 -181 100 0 n#462 inhier.enable.P -1696 -192 -640 -192 -640 320 -448 320 ecalcs.hardware.INPC
w -1262 459 100 0 n#449 eais.custom.VAL -1440 448 -1024 448 ecalcs.multiplex.INPE
w -542 1451 100 0 n#434 eais.window.VAL -1440 1440 416 1440 ecalcs.coincidence.INPA
w 786 1259 100 0 n#422 ecalcs.coincidence.VAL 704 1248 928 1248 outhier.inposition.p
w -610 395 100 0 n#419 ecalcs.multiplex.VAL -736 384 -448 384 ecalcs.hardware.INPA
w 140 459 100 0 n#419 junction -576 384 -576 448 928 448 outhier.demand.p
w -4 923 100 0 n#419 ecalcs.coincidence.INPB 416 1408 0 1408 0 448 junction
s 1168 -624 100 0 1
s 1072 -624 100 0 1
s 816 -624 100 0 26-AUG-96
s 560 -640 100 0 checked: S. Prior
s 560 -608 100 0 author: S.Prior
s 784 -560 100 0 Mechanism Schematic
s 784 -496 100 0 Secondary Control System
s 416 -128 400 0 mechanism
s 608 1024 100 0 CALC: ABS(B-C)>A?1:0
s -416 -224 100 0 CALC: C=0?(A+B)/2
s -1136 16 100 0 CALC: L=0?B+A:(L=1?C+A:(L=2?D+A:E+A))
s 608 1664 100 0 mechanism.sch
[cell use]
use inhier -1712 -41 100 0 select
xform 0 -1696 0
use inhier -1712 -233 100 0 enable
xform 0 -1696 -192
use inhier -1712 151 100 0 SLNK
xform 0 -1696 192
use eais -1696 391 100 0 custom
xform 0 -1568 464
p -1907 537 100 0 0 DESC:custom demand
p -1952 302 100 0 0 PREC:2
p -1600 384 100 0 1 PV:$(top)$(axis)
p -1568 528 100 1024 -1 name:$(top)custom
use eais -1696 583 100 0 c
xform 0 -1568 656
p -1907 729 100 0 0 DESC:beam c demand
p -1952 494 100 0 0 PREC:2
p -1632 576 100 0 1 PV:$(top)$(axis)
p -1584 720 100 1024 -1 name:$(top)c
use eais -1696 775 100 0 b
xform 0 -1568 848
p -1907 921 100 0 0 DESC:beam b demand
p -1952 686 100 0 0 PREC:2
p -1632 768 100 0 1 PV:$(top)$(axis)
p -1584 912 100 1024 -1 name:$(top)b
use eais -1696 967 100 0 a
xform 0 -1568 1040
p -1907 1113 100 0 0 DESC:beam a demand
p -1952 750 100 0 0 HOPR:100
p -1952 718 100 0 0 LOPR:-100
p -1952 878 100 0 0 PREC:2
p -1632 960 100 0 1 PV:$(top)$(axis)
p -1584 1104 100 1024 -1 name:$(top)a
use eais -1696 1159 100 0 guide
xform 0 -1568 1232
p -1907 1305 100 0 0 DESC:guide correction value
p -1952 1070 100 0 0 PREC:5
p -1600 1152 100 0 1 PV:$(top)$(axis)
p -1584 1296 100 1024 -1 name:$(top)guide
use eais -1696 1383 100 0 window
xform 0 -1568 1456
p -1907 1529 100 0 0 DESC:in position window size
p -1952 1198 100 0 0 EGU:none
p -1952 1294 100 0 0 PREC:2
p -1600 1376 100 0 1 PV:$(top)$(axis)
use outhier 896 151 100 0 position
xform 0 912 192
use outhier 896 407 100 0 demand
xform 0 912 448
use outhier 896 1207 100 0 inposition
xform 0 912 1248
use outhier 896 1399 100 0 FLNK
xform 0 912 1440
use bc200tr -2016 -776 -100 0 frame
xform 0 -336 528
use ecalcs -448 -89 100 0 hardware
xform 0 -304 176
p -541 392 100 0 0 CALC:C=0?(A+B)/2
p -655 342 100 0 0 DESC:hardware simulation
p -736 62 100 0 0 EGU:arcsecs
p -736 30 100 0 0 PREC:5
p -320 -96 100 0 1 PV:$(top)$(axis)
p -736 286 100 0 0 SCAN:.1 second
p -368 -128 100 1024 -1 name:$(top)hardware
use ecalcs -1024 103 100 0 multiplex
xform 0 -880 368
p -1117 584 100 0 0 CALC:L=0?B+A:(L=1?C+A:(L=2?D+A:E+A))
p -1231 534 100 0 0 DESC:Multiplex beams
p -1312 254 100 0 0 EGU:arcsecs
p -1312 222 100 0 0 PREC:2
p -896 96 100 0 1 PV:$(top)$(axis)
p -1312 478 100 0 0 SCAN:Passive
p -928 64 100 1024 -1 name:$(top)multiplex
use ecalcs 416 967 100 0 coincidence
xform 0 560 1232
p 323 1448 100 0 0 CALC:ABS(B-C)>A?1:0
p 209 1398 100 0 0 DESC:calculate coincidence
p 213 817 100 0 1 PV:$(top)$(axis)
p 128 1342 100 0 0 SCAN:Passive
p 528 928 100 1024 -1 name:$(top)coincidence
[comments]
