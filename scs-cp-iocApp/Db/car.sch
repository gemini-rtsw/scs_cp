[schematic2]
uniq 271
[tools]
[detail]
w 2258 2411 100 0 n#270 hwin.hwin#269.in 2272 2400 2304 2400 egenSub.allCar2.INPF
w 514 2283 100 0 n#268 hwin.hwin#267.in 544 2272 544 2272 egenSub.allCar.INPH
w 1890 2763 100 0 n#264 egenSub.allCar1.VALA 1792 2752 2048 2752 2048 2736 2304 2736 2304 2720 egenSub.allCar2.INPA
w 1906 2027 100 0 n#263 egenSub.allCar1.FLNK 1792 2016 2080 2016 2080 2048 2304 2048 egenSub.allCar2.SLNK
w 994 2755 100 0 n#261 egenSub.allCar.VALA 832 2752 1216 2752 1216 2720 1504 2720 egenSub.allCar1.INPA
w 1026 2019 100 0 n#260 egenSub.allCar.FLNK 832 2016 1280 2016 1280 2048 1504 2048 egenSub.allCar1.SLNK
w 1474 2155 100 0 n#248 egenSub.allCar1.INPJ 1504 2144 1504 2144 hwin.hwin#259.in
w 1474 2219 100 0 n#247 egenSub.allCar1.INPI 1504 2208 1504 2208 hwin.hwin#258.in
w 1474 2283 100 0 n#246 egenSub.allCar1.INPH 1504 2272 1504 2272 hwin.hwin#257.in
w 1474 2347 100 0 n#245 egenSub.allCar1.INPG 1504 2336 1504 2336 hwin.hwin#256.in
w 1474 2411 100 0 n#244 egenSub.allCar1.INPF 1504 2400 1504 2400 hwin.hwin#255.in
w 1474 2475 100 0 n#243 egenSub.allCar1.INPE 1504 2464 1504 2464 hwin.hwin#254.in
w 1474 2603 100 0 n#241 egenSub.allCar1.INPC 1504 2592 1504 2592 hwin.hwin#252.in
w 1474 2667 100 0 n#240 egenSub.allCar1.INPB 1504 2656 1504 2656 hwin.hwin#251.in
w 2626 2731 100 0 n#238 egenSub.allCar2.OUTA 2592 2720 2720 2720 2720 2752 2816 2752 ecars.ecars#155.IVAL
w 2756 2283 100 0 n#237 egenSub.allCar2.FLNK 2592 2016 2752 2016 2752 2560 2816 2560 ecars.ecars#155.SLNK
w 2274 2475 100 0 n#218 egenSub.allCar2.INPE 2304 2464 2304 2464 hwin.hwin#229.in
w 2274 2539 100 0 n#217 egenSub.allCar2.INPD 2304 2528 2304 2528 hwin.hwin#228.in
w 2274 2603 100 0 n#216 egenSub.allCar2.INPC 2304 2592 2304 2592 hwin.hwin#227.in
w 2274 2667 100 0 n#215 egenSub.allCar2.INPB 2304 2656 2304 2656 hwin.hwin#226.in
w 514 2155 100 0 n#208 hwin.hwin#207.in 544 2144 544 2144 egenSub.allCar.INPJ
w 514 2219 100 0 n#206 hwin.hwin#205.in 544 2208 544 2208 egenSub.allCar.INPI
w 514 2347 100 0 n#202 hwin.hwin#201.in 544 2336 544 2336 egenSub.allCar.INPG
w 514 2411 100 0 n#200 hwin.hwin#199.in 544 2400 544 2400 egenSub.allCar.INPF
w 514 2475 100 0 n#198 hwin.hwin#197.in 544 2464 544 2464 egenSub.allCar.INPE
w 514 2539 100 0 n#196 hwin.hwin#195.in 544 2528 544 2528 egenSub.allCar.INPD
w 514 2603 100 0 n#194 hwin.hwin#193.in 544 2592 544 2592 egenSub.allCar.INPC
w 514 2667 100 0 n#192 hwin.hwin#191.in 544 2656 544 2656 egenSub.allCar.INPB
w 514 2731 100 0 n#190 hwin.hwin#189.in 544 2720 544 2720 egenSub.allCar.INPA
s 2816 752 100 0 24-Jun-98
s 2544 720 100 0 checked: S.Prior
s 2544 752 100 0 author: S.Prior
s 3152 736 100 0 1
s 3056 736 100 0 1
s 2784 800 100 0 OR together all CAR responses
s 2784 864 100 0 Secondary Control System
s 2608 3024 100 0 car.sch
[cell use]
use hwin 2080 2359 100 0 hwin#269
xform 0 2176 2400
p 1904 2352 100 0 -1 val(in):$(top)setVTKControlC.VAL
use hwin 352 2679 100 0 hwin#189
xform 0 448 2720
p 144 2720 100 0 -1 val(in):$(top)debugC.VAL
use hwin 352 2615 100 0 hwin#191
xform 0 448 2656
p 128 2640 100 0 -1 val(in):$(top)followC.VAL
use hwin 352 2551 100 0 hwin#193
xform 0 448 2592
p 80 2592 100 0 -1 val(in):$(top)decsConfigC.VAL
use hwin 352 2487 100 0 hwin#195
xform 0 448 2528
p 48 2512 100 0 -1 val(in):$(top)setControllerC.VAL
use hwin 352 2423 100 0 hwin#197
xform 0 448 2464
p 80 2448 100 0 -1 val(in):$(top)moveBaffleC.VAL
use hwin 352 2359 100 0 hwin#199
xform 0 448 2400
p 160 2384 100 0 -1 val(in):$(top)simC.VAL
use hwin 352 2295 100 0 hwin#201
xform 0 448 2336
p 144 2336 100 0 -1 val(in):$(top)initC.VAL
use hwin 352 2167 100 0 hwin#205
xform 0 448 2208
p 96 2240 100 0 -1 val(in):$(top)moveActuatorC.VAL
use hwin 352 2103 100 0 hwin#207
xform 0 448 2144
p 144 2144 100 0 -1 val(in):$(top)stopC.VAL
use hwin 2112 2615 100 0 hwin#226
xform 0 2208 2656
p 1904 2640 100 0 -1 val(in):$(top)datumC.VAL
use hwin 2112 2551 100 0 hwin#227
xform 0 2208 2592
p 1920 2576 100 0 -1 val(in):$(top)testC.VAL
use hwin 2112 2487 100 0 hwin#228
xform 0 2208 2528
p 1904 2528 100 0 -1 val(in):$(top)rebootC.VAL
use hwin 2112 2423 100 0 hwin#229
xform 0 2208 2464
p 1920 2464 100 0 -1 val(in):$(top)logC.VAL
use hwin 1312 2615 100 0 hwin#251
xform 0 1408 2656
p 1120 2640 100 0 -1 val(in):$(top)parkC.VAL
use hwin 1312 2551 100 0 hwin#252
xform 0 1408 2592
p 1120 2592 100 0 -1 val(in):$(top)moveC.VAL
use hwin 1312 2423 100 0 hwin#254
xform 0 1408 2464
p 1072 2464 100 0 -1 val(in):$(top)toleranceC.VAL
use hwin 1312 2359 100 0 hwin#255
xform 0 1408 2400
p 1008 2384 100 0 -1 val(in):$(top)servoBandwidthC.VAL
use hwin 1312 2295 100 0 hwin#256
xform 0 1408 2336
p 1040 2336 100 0 -1 val(in):$(top)guideConfigC.VAL
use hwin 1312 2231 100 0 hwin#257
xform 0 1408 2272
p 1104 2256 100 0 -1 val(in):$(top)guideC.VAL
use hwin 1312 2167 100 0 hwin#258
xform 0 1408 2208
p 1040 2208 100 0 -1 val(in):$(top)chopControlC.VAL
use hwin 1312 2103 100 0 hwin#259
xform 0 1408 2144
p 1056 2144 100 0 -1 val(in):$(top)chopConfigC.VAL
use hwin 352 2231 100 0 hwin#267
xform 0 448 2272
p 112 2272 100 0 -1 val(in):$(top)beamJogC.VAL
use egenSub 544 1959 100 0 allCar
xform 0 688 2384
p 387 2699 100 0 0 DESC:or together all car outputs
p 321 1733 100 0 0 FTA:LONG
p 321 1733 100 0 0 FTB:LONG
p 321 1701 100 0 0 FTC:LONG
p 321 1669 100 0 0 FTD:LONG
p 321 1637 100 0 0 FTE:LONG
p 321 1573 100 0 0 FTF:LONG
p 321 1573 100 0 0 FTG:LONG
p 321 1541 100 0 0 FTH:LONG
p 321 1509 100 0 0 FTI:LONG
p 321 1477 100 0 0 FTJ:LONG
p 321 1733 100 0 0 FTVA:LONG
p 544 1840 100 0 1 INAM:dummyInitGenSub
p 544 1904 100 0 1 PV:$(top)
p 560 1776 100 0 1 SNAM:carDrive1
use egenSub 2304 1959 100 0 allCar2
xform 0 2448 2384
p 2147 2699 100 0 0 DESC:or together all car outputs
p 2081 1733 100 0 0 FTA:LONG
p 2081 1733 100 0 0 FTB:LONG
p 2081 1701 100 0 0 FTC:LONG
p 2081 1669 100 0 0 FTD:LONG
p 2081 1637 100 0 0 FTE:LONG
p 2081 1573 100 0 0 FTF:LONG
p 2081 1573 100 0 0 FTG:LONG
p 2081 1541 100 0 0 FTH:LONG
p 2081 1509 100 0 0 FTI:LONG
p 2081 1477 100 0 0 FTJ:LONG
p 2081 1733 100 0 0 FTVA:LONG
p 2336 1808 100 0 1 INAM:dummyInitGenSub
p 2304 1904 100 0 1 PV:$(top)
p 2368 1760 100 0 1 SNAM:carDrive3
use egenSub 1504 1959 100 0 allCar1
xform 0 1648 2384
p 1347 2699 100 0 0 DESC:or together all car outputs
p 1281 1733 100 0 0 FTA:LONG
p 1281 1733 100 0 0 FTB:LONG
p 1281 1701 100 0 0 FTC:LONG
p 1281 1669 100 0 0 FTD:LONG
p 1281 1637 100 0 0 FTE:LONG
p 1281 1573 100 0 0 FTF:LONG
p 1281 1573 100 0 0 FTG:LONG
p 1281 1541 100 0 0 FTH:LONG
p 1281 1509 100 0 0 FTI:LONG
p 1281 1477 100 0 0 FTJ:LONG
p 1281 1733 100 0 0 FTVA:LONG
p 1504 1840 100 0 1 INAM:dummyInitGenSub
p 1504 1904 100 0 1 PV:$(top)
p 1552 1776 100 0 1 SNAM:carDrive2
use ecars 2816 2471 100 0 ecars#155
xform 0 2976 2640
p 2767 2400 100 0 0 DESC:active C CAR
p 2896 2832 100 0 -1 name:$(top)activeC
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
