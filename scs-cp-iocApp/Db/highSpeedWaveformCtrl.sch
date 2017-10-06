[schematic2]
uniq 16
[tools]
[detail]
w 344 523 100 0 FLNK ecalcs.wfcC3.FLNK 288 512 448 512 448 544 512 544 outhier.FLNK.p
w 376 491 100 0 VAL ecalcs.wfcC3.VAL 288 480 512 480 outhier.VAL.p
w -72 651 100 0 n#13 hwin.hwin#12.in -96 640 0 640 ecalcs.wfcC3.INPB
w -200 683 100 0 n#11 junction -352 672 0 672 ecalcs.wfcC3.INPA
w -600 747 100 0 n#11 ecalcs.wfcC2.VAL -416 480 -352 480 -352 736 -800 736 -800 672 -704 672 ecalcs.wfcC2.INPA
w -920 491 100 0 n#10 ecalcs.wfcC1.VAL -992 480 -800 480 -800 640 -704 640 ecalcs.wfcC2.INPB
w -322 523 100 0 n#9 ecalcs.wfcC2.FLNK -416 512 -192 512 -192 288 0 288 ecalcs.wfcC3.SLNK
w -930 523 100 0 n#8 ecalcs.wfcC1.FLNK -992 512 -832 512 -832 288 -704 288 ecalcs.wfcC2.SLNK
w -1394 683 100 0 n#5 hwin.hwin#4.in -1504 736 -1472 736 -1472 672 -1280 672 ecalcs.wfcC1.INPA
w -1426 299 100 0 SLNK inhier.SLNK.P -1536 288 -1280 288 ecalcs.wfcC1.SLNK
w -1438 651 100 0 SRATE inhier.SRATE.P -1536 640 -1280 640 ecalcs.wfcC1.INPB
s 64 64 100 0 when counter1 equals 0
s 64 96 100 0 Output 1 (ENABLE)
s 64 128 100 0 if counter1 is less than SRATE
s -48 160 100 0 Counter2: Ouput 0 (DISABLE) 
s -752 160 100 0 Counter: from 0 to SRATE-1
s 800 -944 100 0 M. Rippa
s 800 -880 100 0 HighSpeed WF Control
[cell use]
use outhier 480 503 100 0 FLNK
xform 0 496 544
use outhier 480 439 100 0 VAL
xform 0 496 480
use hwin -288 599 100 0 hwin#12
xform 0 -192 640
p -285 632 100 0 -1 val(in):0
use ecalcs 0 199 100 0 wfcC3
xform 0 144 464
p 96 352 100 0 1 CALC:A>B?0:1
use ecalcs -704 199 100 0 wfcC2
xform 0 -560 464
p -624 352 100 0 1 CALC:A<B?A+1:0
use hwin -1696 695 100 0 hwin#4
xform 0 -1600 736
p -1693 728 100 0 -1 val(in):1
use ecalcs -1280 199 100 0 wfcC1
xform 0 -1136 464
p -1184 416 100 0 1 CALC:B-1
use inhier -1552 247 100 0 SLNK
xform 0 -1536 288
use inhier -1552 599 100 0 SRATE
xform 0 -1536 640
use bc200tr -2032 -1160 -100 0 frame
xform 0 -352 144
[comments]
