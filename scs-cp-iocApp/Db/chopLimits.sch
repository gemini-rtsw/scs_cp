[schematic2]
uniq 207
[tools]
[detail]
w 1154 2795 100 0 n#181 inhier.profile.P 1088 2784 1280 2784 egenSub.envelope.A
w 1764 1547 100 0 n#191 egenSub.envelope.OUTJ 1568 2176 1760 2176 1760 928 2112 928 eaos.eaos#153.SLNK
w 1796 1675 100 0 n#179 eaos.eaos#152.SLNK 2112 1120 1792 1120 1792 2240 1568 2240 egenSub.envelope.OUTI
w 1828 1803 100 0 n#178 egenSub.envelope.OUTH 1568 2304 1824 2304 1824 1312 2112 1312 eaos.eaos#151.SLNK
w 1860 1947 100 0 n#177 eaos.eaos#150.SLNK 2112 1536 1856 1536 1856 2368 1568 2368 egenSub.envelope.OUTG
w 1892 2075 100 0 n#176 egenSub.envelope.OUTF 1568 2432 1888 2432 1888 1728 2112 1728 eaos.eaos#149.SLNK
w 1924 2219 100 0 n#175 egenSub.envelope.OUTE 1568 2496 1920 2496 1920 1952 2112 1952 eaos.eaos#148.SLNK
w 1956 2347 100 0 n#174 egenSub.envelope.OUTD 1568 2560 1952 2560 1952 2144 2112 2144 eaos.eaos#147.SLNK
w 1746 2635 100 0 n#173 egenSub.envelope.OUTC 1568 2624 1984 2624 1984 2336 2112 2336 eaos.eaos#146.SLNK
w 1762 2699 100 0 n#172 egenSub.envelope.OUTB 1568 2688 2016 2688 2016 2528 2112 2528 eaos.eaos#145.SLNK
w 1810 2763 100 0 n#170 egenSub.envelope.OUTA 1568 2752 2112 2752 eaos.eaos#143.SLNK
s 160 1536 200 0 and makes the parameters available to the TCS
s 160 1584 200 0 the limits when a new profile is selected
s 160 1632 200 0 to the selected profile. This record calculates
s 160 1696 200 0 The limits of M2 mirror chopping vary according
s 2784 864 100 0 Secondary Control System
s 2784 800 100 0 Chop Limits
s 3056 736 100 0 1
s 3152 736 100 0 1
s 2544 752 100 0 author: S.Prior
s 2544 720 100 0 checked: S.Prior
s 2816 752 100 0 01-Mar-98
s 2592 3024 100 0 chopLimits.sch
[cell use]
use inhier 1072 2743 100 0 profile
xform 0 1088 2784
use egenSub 1280 1991 100 0 envelope
xform 0 1424 2416
p 1123 2731 100 0 0 DESC:chop envelope limits
p 1057 1765 100 0 0 FTA:LONG
p 1328 1872 100 0 1 INAM:dummyInitGenSub
p 992 2590 100 0 0 PINI:NO
p 992 2542 100 0 0 PREC:4
p 1344 1936 100 0 1 PV:$(top)
p 992 2686 100 0 0 SCAN:Passive
p 1328 1904 100 0 1 SNAM:calcEnvelope
p 1057 2149 100 0 0 def(OUTA):0
p 1057 2117 100 0 0 def(OUTB):0
p 1057 2085 100 0 0 def(OUTC):0
p 1232 2762 75 0 -1 pproc(INPA):NPP
p 1232 2698 75 0 -1 pproc(INPB):NPP
p 1232 2634 75 0 -1 pproc(INPC):NPP
p 1232 2570 75 0 -1 pproc(INPD):NPP
p 1232 2506 75 0 -1 pproc(INPE):NPP
p 1232 2442 75 0 -1 pproc(INPF):NPP
p 1232 2378 75 0 -1 pproc(INPG):NPP
p 1232 2314 75 0 -1 pproc(INPH):NPP
p 1232 2250 75 0 -1 pproc(INPI):NPP
p 1232 2186 75 0 -1 pproc(INPJ):NPP
p 1568 2762 75 0 -1 pproc(OUTA):PP
p 1568 2698 75 0 -1 pproc(OUTB):PP
p 1568 2634 75 0 -1 pproc(OUTC):PP
p 1568 2570 75 0 -1 pproc(OUTD):PP
p 1568 2506 75 0 -1 pproc(OUTE):PP
p 1568 2442 75 0 -1 pproc(OUTF):PP
p 1568 2378 75 0 -1 pproc(OUTG):PP
p 1568 2314 75 0 -1 pproc(OUTH):PP
p 1568 2250 75 0 -1 pproc(OUTI):PP
p 1568 2186 75 0 -1 pproc(OUTJ):PP
use eaos 2112 2663 100 0 eaos#143
xform 0 2240 2752
p 2080 2478 100 0 0 EGU:Hz
p 1856 2734 100 0 0 OMSL:closed_loop
p 1856 2606 100 0 0 PREC:2
p 2176 2640 100 0 -1 name:$(top)breakChopFreq
use eaos 2112 2439 100 0 eaos#145
xform 0 2240 2528
p 2080 2254 100 0 0 EGU:Hz
p 1856 2510 100 0 0 OMSL:closed_loop
p 1856 2382 100 0 0 PREC:2
p 2176 2416 100 0 -1 name:$(top)breakChopFreqThrow
use eaos 2112 2247 100 0 eaos#146
xform 0 2240 2336
p 2080 2062 100 0 0 EGU:Hz
p 1856 2318 100 0 0 OMSL:closed_loop
p 1856 2190 100 0 0 PREC:2
p 2176 2224 100 0 -1 name:$(top)maxChopFreq
use eaos 2112 2055 100 0 eaos#147
xform 0 2240 2144
p 2080 1870 100 0 0 EGU:Hz
p 1856 1998 100 0 0 PREC:2
p 2176 2032 100 0 -1 name:$(top)maxChopFreqThrow
use eaos 2112 1863 100 0 eaos#148
xform 0 2240 1952
p 2080 1678 100 0 0 EGU:arcsecs
p 1856 1806 100 0 0 PREC:2
p 2176 1840 100 0 -1 name:$(top)maxTilt
use eaos 2112 1639 100 0 eaos#149
xform 0 2240 1728
p 2080 1454 100 0 0 EGU:microns
p 1856 1582 100 0 0 PREC:2
p 2176 1616 100 0 -1 name:$(top)maxTiltZ
use eaos 2112 1447 100 0 eaos#150
xform 0 2240 1536
p 2080 1262 100 0 0 EGU:Hz
p 1856 1390 100 0 0 PREC:2
p 2176 1424 100 0 -1 name:$(top)minChopFreq
use eaos 2112 1223 100 0 eaos#151
xform 0 2240 1312
p 2080 1038 100 0 0 EGU:Hz
p 1856 1166 100 0 0 PREC:2
p 2176 1200 100 0 -1 name:$(top)minChopFreqThrow
use eaos 2112 1031 100 0 eaos#152
xform 0 2240 1120
p 2080 846 100 0 0 EGU:arcsecs
p 1856 974 100 0 0 PREC:2
p 2176 1008 100 0 -1 name:$(top)minTilt
use eaos 2112 839 100 0 eaos#153
xform 0 2240 928
p 2080 654 100 0 0 EGU:microns
p 1856 782 100 0 0 PREC:2
p 2176 816 100 0 -1 name:$(top)minTiltZ
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
