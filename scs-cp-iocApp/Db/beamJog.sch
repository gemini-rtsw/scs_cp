[schematic2]
uniq 204
[tools]
[detail]
w 1458 2091 100 0 n#201 hwin.hwin#200.in 1488 2080 1488 2080 eseqs.beamJogSeq.DOL2
w 1834 2091 100 0 n#199 eseqs.beamJogSeq.LNK2 1808 2080 1920 2080 1920 2112 junction
w 1994 2123 100 0 n#199 eseqs.beamJogSeq.LNK1 1808 2112 2240 2112 ecars.beamJogC.IVAL
w 1458 2123 100 0 n#198 hwin.hwin#197.in 1488 2112 1488 2112 eseqs.beamJogSeq.DOL1
w 1218 1739 100 0 n#192 ecad2.beamJog.STLK 1152 1728 1344 1728 1344 1792 1488 1792 eseqs.beamJogSeq.SLNK
w 2530 1899 100 0 n#179 hwout.hwout#178.outp 2560 1888 2560 1888 ecars.beamJogC.FLNK
s 2592 3024 100 0 baffle.sch
s 2784 864 100 0 Secondary Control System
s 2784 800 100 0 Select chop beam for tweaking
s 3056 736 100 0 1
s 3152 736 100 0 1
s 2544 752 100 0 author: S.Prior
s 2544 720 100 0 checked: S.Prior
s 2816 752 100 0 29-Jun-98
[cell use]
use hwout 2560 1847 100 0 hwout#178
xform 0 2656 1888
p 2656 1879 100 0 -1 val(outp):$(top)allCar.VAL
use hwin 1296 2039 100 0 hwin#200
xform 0 1392 2080
p 1299 2072 100 0 -1 val(in):0
use hwin 1296 2071 100 0 hwin#197
xform 0 1392 2112
p 1299 2104 100 0 -1 val(in):2
use eseqs 1488 1703 100 0 beamJogSeq
xform 0 1648 1952
p 1568 1632 100 0 0 DLY2:1
p 1552 1664 100 0 1 PV:$(top)
p 1824 2112 75 1024 -1 pproc(LNK1):PP
p 1824 2080 75 1024 -1 pproc(LNK2):PP
use ecad2 832 1639 100 0 beamJog
xform 0 992 1952
p 896 2696 100 0 0 DESC:force chop beam position
p 896 2440 100 0 0 FTVA:LONG
p 896 2408 100 0 0 FTVB:LONG
p 896 1568 100 0 1 PV:$(top)
p 896 2728 100 0 0 SNAM:CADbeamJog
p 912 2288 100 0 0 name:$(top)$(I)
use ecars 2240 1831 100 0 beamJogC
xform 0 2400 2000
p 2240 1792 100 0 1 PV:$(top)
p 2288 2176 100 0 0 name:$(top)$(I)
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
