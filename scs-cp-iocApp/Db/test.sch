[schematic2]
uniq 175
[tools]
[detail]
w 580 2475 100 0 n#174 ecad2.test.STLK 448 2400 576 2400 576 2560 704 2560 eseqs.testSeq.SLNK
w 1124 2619 100 0 n#173 eseqs.testSeq.LNK2 1024 2848 1120 2848 1120 2400 1280 2400 cadplus.testP.STIN
w 850 2339 100 0 n#171 ecad2.test.VAL 448 2880 480 2880 480 2336 1280 2336 cadplus.testP.STAT
w 834 2371 100 0 n#170 ecad2.test.SPLK 448 2368 1280 2368 cadplus.testP.SPIN
w 1090 2891 100 0 n#169 eseqs.testSeq.LNK1 1024 2880 1216 2880 ecars.testC.IVAL
w 674 2891 100 0 n#164 eseqs.testSeq.DOL1 704 2880 704 2880 hwin.hwin#167.in
w 2322 1090 100 0 n#163 ecad2.ecad2#126.VALA 1792 1376 2016 1376 2016 1088 2688 1088 estringouts.debugLevel.DOL
w 2562 1166 100 0 n#162 eseqs.debugSeq.FLNK 2464 1216 2560 1216 2560 1056 2688 1056 estringouts.debugLevel.SLNK
w 1858 1090 100 0 n#161 ecad2.ecad2#126.STLK 1792 1088 1984 1088 1984 1216 2144 1216 eseqs.debugSeq.SLNK
w 1042 1406 100 0 n#159 embbos.debugPick.VAL 960 1328 1040 1328 1040 1424 1120 1424 estringouts.debugDemand.DOL
w 1010 1394 100 0 n#158 embbos.debugPick.FLNK 960 1392 1120 1392 estringouts.debugDemand.SLNK
w 1394 1378 100 0 n#157 estringouts.debugDemand.OUT 1376 1376 1472 1376 ecad2.ecad2#126.A
w 2114 1515 100 0 n#155 hwin.hwin#154.in 2144 1504 2144 1504 eseqs.debugSeq.DOL2
w 2114 1547 100 0 n#153 hwin.hwin#152.in 2144 1536 2144 1536 eseqs.debugSeq.DOL1
w 2530 1539 100 0 n#151 eseqs.debugSeq.LNK1 2464 1536 2656 1536 ecars.ecars#128.IVAL
w 2498 1515 100 0 n#151 eseqs.debugSeq.LNK2 2464 1504 2592 1504 2592 1536 junction
w 1860 2139 100 0 n#141 embbos.logChoice.OUT 1760 1920 1856 1920 1856 2368 1984 2368 ecad8.log.E
w 2962 1315 100 0 n#121 ecars.ecars#128.FLNK 2976 1312 3008 1312 hwout.hwout#133.outp
w 2914 2603 100 0 n#103 hwout.hwout#102.outp 2944 2592 2944 2592 ecars.logC.FLNK
w 1506 2667 100 0 n#101 hwout.hwout#100.outp 1536 2656 1536 2656 ecars.testC.FLNK
w 2500 2347 100 0 n#94 cadplus.cadplus#89.STAT 2592 1888 2496 1888 2496 2816 2304 2816 ecad8.log.VAL
w 2424 1931 100 0 n#93 ecad8.log.SPLK 2304 1920 2592 1920 cadplus.cadplus#89.SPIN
w 2424 1963 100 0 n#92 ecad8.log.STLK 2304 1952 2592 1952 cadplus.cadplus#89.STIN
s 2784 864 100 0 Secondary Control System
s 2784 800 100 0 Test Commands
s 3056 736 100 0 1
s 3152 736 100 0 1
s 2544 752 100 0 author: S.Prior
s 2544 720 100 0 checked: S.Prior
s 2816 752 100 0 02-Jul-98
s 160 896 200 0 CAD records to specify test functions
s 160 832 200 0 The test command initiates testing for the SCS and tilt systems
s 160 768 200 0 debug and log apply only to the SCS
s 2544 3056 100 0 $Id: test.sch,v 1.2 2000/02/05 03:30:06 dayle Exp $
[cell use]
use hwin 1952 1495 100 0 hwin#152
xform 0 2048 1536
p 1955 1528 100 0 -1 val(in):2
use hwin 1952 1463 100 0 hwin#154
xform 0 2048 1504
p 1955 1496 100 0 -1 val(in):0
use hwin 512 2839 100 0 hwin#167
xform 0 608 2880
p 515 2872 100 0 -1 val(in):2
use eseqs 2144 1127 100 0 debugSeq
xform 0 2304 1376
p 2224 1056 100 0 0 DLY2:1
p 2480 1536 75 1024 -1 pproc(LNK1):PP
p 2480 1504 75 1024 -1 pproc(LNK2):PP
use eseqs 704 2471 100 0 testSeq
xform 0 864 2720
p 784 2400 100 0 0 DLY2:1
p 1172 1708 100 0 1 PV:$(top)
p 1040 2880 75 1024 -1 pproc(LNK1):PP
p 1040 2848 75 1024 -1 pproc(LNK2):NPP
use estringouts 224 2055 100 0 fault0
xform 0 352 2128
p 251 2168 100 0 0 DESC:M2 system diagnostic string
p 224 2032 100 0 1 PV:$(top)
use estringouts 1144 1320 100 0 debugDemand
xform 0 1248 1392
p 1168 1470 100 0 -1 DESC:Debug level string
p 1056 1134 100 0 0 IVOV:None
p 1056 1198 100 0 0 OMSL:closed_loop
p 1056 1358 100 0 0 VAL:None
use estringouts 2712 984 100 0 debugLevel
xform 0 2816 1056
p 2736 1134 100 0 -1 DESC:Current debug level
p 2624 798 100 0 0 IVOV:None
p 2624 862 100 0 0 OMSL:closed_loop
p 2624 1022 100 0 0 VAL:
use embbos 1504 1831 100 0 logChoice
xform 0 1632 1920
p 1520 1792 100 0 1 PV:$(top)
use embbos 704 1271 100 0 debugPick
xform 0 832 1360
p 752 1470 100 0 -1 DESC:Debug level options
p 672 1088 100 0 1 FRST:RESERVED1
p 672 1056 100 0 1 FVST:RESERVED2
p 864 1198 100 0 1 ONST:MIN
p 672 1198 100 0 1 ONVL:1
p 672 1120 100 0 1 THST:MAX
p 672 846 100 0 0 TVVL:2
p 864 1166 100 0 1 TWST:MED
p 672 1166 100 0 1 TWVL:2
p 864 1230 100 0 1 ZRST:NONE
p 672 1230 100 0 1 ZRVL:0
use hwout 3008 1271 100 0 hwout#133
xform 0 3104 1312
p 3056 1248 100 0 -1 val(outp):$(top)allCar
use hwout 1536 2615 100 0 hwout#100
xform 0 1632 2656
p 1488 2608 100 0 1 val(outp):$(top)allCar.VAL
use hwout 2944 2551 100 0 hwout#102
xform 0 3040 2592
p 3040 2583 100 0 -1 val(outp):$(top)allCar2.VAL
use ecars 2656 1255 100 0 ecars#128
xform 0 2816 1424
p 2816 1232 100 1024 1 name:$(top)debugC
use ecars 2624 2535 100 0 logC
xform 0 2784 2704
p 2612 2284 100 0 0 PV:$(top)
use ecars 1216 2599 -100 0 testC
xform 0 1376 2768
p 1167 2528 100 0 0 DESC:test command CAR record
p 1312 2560 100 0 -1 name:$(top)testC
use cadplus 2592 1799 100 0 cadplus#89
xform 0 2656 1904
p 2592 1776 100 0 1 seta:plus $(top)logP:
use cadplus 1280 2247 100 0 testP
xform 0 1344 2352
p 1280 2448 100 0 1 seta:plus $(top)testP:
use ecad2 1472 999 100 0 ecad2#126
xform 0 1632 1312
p 1536 1630 100 0 -1 DESC:SCS debug command
p 1568 1312 100 0 0 FTVA:STRING
p 1568 1024 100 0 0 SNAM:CADdebug
p 1632 960 100 1024 1 name:$(top)debug
p 1424 1344 75 1024 -1 pproc(INPA):NPP
use ecad2 128 2311 -100 0 test
xform 0 288 2624
p 192 3368 100 0 0 DESC:test command
p 192 3400 100 0 0 SNAM:CADtest
p 192 2216 100 0 0 def(STLK):0.0
p 208 2256 100 0 -1 name:$(top)test
use ecad8 1984 1863 100 0 log
xform 0 2144 2368
p 2080 2720 100 0 0 DESC:data logging specification
p 2080 2560 100 0 0 FTVA:LONG
p 2080 2528 100 0 0 FTVB:LONG
p 2080 2496 100 0 0 FTVC:LONG
p 2080 2464 100 0 0 FTVD:LONG
p 2080 2432 100 0 0 FTVE:LONG
p 2080 2400 100 0 0 FTVF:STRING
p 2004 1036 100 0 0 PV:$(top)
p 2048 1792 100 0 1 SNAM:CADlog
p 2304 2272 75 768 -1 pproc(OUTF):PP
use elongouts 640 2023 100 0 testResponse
xform 0 768 2112
p 656 2208 100 0 -1 name:$(top)testResponse
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
