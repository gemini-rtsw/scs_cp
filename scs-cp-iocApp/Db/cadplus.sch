[schematic2]
uniq 43
[tools]
[detail]
w -72 -821 100 0 STAT inhier.STAT.P -192 -832 96 -832 96 -736 256 -736 ecalcs.stop.INPB
w 100 -437 100 0 STAT junction 96 -736 96 -128 240 -128 ecalcs.start.INPB
w 648 -885 100 0 SPID junction 576 -896 768 -896 outhier.SPID.p
w 360 -637 100 0 SPID ecalcs.stop.VAL 544 -896 576 -896 576 -640 192 -640 192 -704 256 -704 ecalcs.stop.INPA
w 648 -277 100 0 STID junction 576 -288 768 -288 outhier.STID.p
w 360 -21 100 0 STID ecalcs.start.VAL 528 -288 576 -288 576 -32 192 -32 192 -96 240 -96 ecalcs.start.INPA
w 60 -885 100 0 SPIN inhier.SPIN.P -192 -672 0 -672 0 -1088 256 -1088 ecalcs.stop.SLNK
w 0 -477 100 0 STIN inhier.STIN.P -192 -480 240 -480 ecalcs.start.SLNK
s 1600 -1552 100 0 Core Instrument Control System
s 1600 -1616 100 0 CAD start and stop counter
s 1600 -1680 100 0 21 Jun 96
s 1344 -1664 100 0 author:S.M.Beard
s 1344 -1696 100 0 checked:S.M.Beard
s 1568 -1664 100 0 C
s 896 -1648 300 0 cadplus.sch
n -288 -416 64 -64 100
This represents the contents
of a hierarchical symbol
"cadplus", to be used to
count the number of START
and STOP activations of each
CAD record. 
_
[cell use]
use bc200tr -1232 -1832 -100 0 frame
xform 0 448 -528
use outhier 736 -937 100 0 SPID
xform 0 752 -896
use outhier 736 -329 100 0 STID
xform 0 752 -288
use inhier -208 -873 100 0 STAT
xform 0 -192 -832
use inhier -208 -713 100 0 SPIN
xform 0 -192 -672
use inhier -208 -521 100 0 STIN
xform 0 -192 -480
use ecalcs 256 -1177 100 0 stop
xform 0 400 -912
p 320 -1200 100 0 1 CALC:B=0?A+1:A
p 448 -1120 100 0 1 PV:$(plus)
use ecalcs 240 -569 100 0 start
xform 0 384 -304
p 304 -592 100 0 1 CALC:B=0?A+1:A
p 448 -480 100 0 1 PV:$(plus)
[comments]
