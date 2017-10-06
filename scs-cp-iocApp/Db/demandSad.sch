[schematic2]
uniq 63
[tools]
[detail]
w 1960 2155 100 0 n#62 hwin.hwin#61.in 1984 2144 1984 2144 esirs.yDemand.INP
w 1960 2507 100 0 n#60 hwin.hwin#59.in 1984 2496 1984 2496 esirs.xDemand.INP
w 1960 3227 100 0 n#58 hwin.hwin#55.in 1984 3216 1984 3216 esirs.yTiltDemand.INP
w 1960 2843 100 0 n#57 hwin.hwin#56.in 1984 2832 1984 2832 esirs.zFocusDemand.INP
w 1960 3579 100 0 n#54 hwin.hwin#53.in 1984 3568 1984 3568 esirs.xTiltDemand.INP
s 3408 3824 100 0 demandSad.sch
s 3616 1552 100 0 23-Dec-97
s 3984 1536 100 0 1
s 3856 1536 100 0 1
s 3360 1520 100 0 checked: S.Prior
s 3376 1552 100 0 author: S.Prior
s 3600 1600 100 0 SCS SIR Records
s 3600 1664 100 0 Secondary Control System
[cell use]
use hwin 1792 3527 100 0 hwin#53
xform 0 1888 3568
p 1584 3504 100 0 1 val(in):$(top)decimator.VALG
use hwin 1792 3175 100 0 hwin#55
xform 0 1888 3216
p 1584 3152 100 0 1 val(in):$(top)decimator.VALH
use hwin 1792 2791 100 0 hwin#56
xform 0 1888 2832
p 1584 2768 100 0 1 val(in):$(top)decimator.VALI
use hwin 1792 2455 100 0 hwin#59
xform 0 1888 2496
p 1584 2432 100 0 1 val(in):$(top)decimator.VALM
use hwin 1792 2103 100 0 hwin#61
xform 0 1888 2144
p 1584 2080 100 0 1 val(in):$(top)decimator.VALN
use esirs 1984 1895 100 0 yDemand
xform 0 2192 2048
p 1935 1472 100 0 0 EGU:microns
p 1935 1568 100 0 0 FTVL:DOUBLE
p 1935 1536 100 0 0 PREC:2
p 2176 1760 100 0 0 SCAN:1 second
p 2128 2192 100 0 -1 name:$(top)yPosDmd
use esirs 1984 2247 100 0 xDemand
xform 0 2192 2400
p 1935 1824 100 0 0 EGU:microns
p 1935 1920 100 0 0 FTVL:DOUBLE
p 1935 1888 100 0 0 PREC:2
p 2176 2112 100 0 0 SCAN:1 second
p 2112 2544 100 0 -1 name:$(top)xPosDmd
use esirs 1984 2583 100 0 zFocusDemand
xform 0 2192 2736
p 1935 2480 100 0 0 DESC:z focus demand
p 1935 2160 100 0 0 EGU:microns
p 1935 2464 100 0 0 FDSC:z focus demand
p 1935 2256 100 0 0 FTVL:DOUBLE
p 1935 2224 100 0 0 PREC:2
p 2176 2448 100 0 0 SCAN:.05 second
p 2112 2896 100 0 -1 name:$(top)zNetDmd
use esirs 1984 2967 100 0 yTiltDemand
xform 0 2192 3120
p 1935 2864 100 0 0 DESC:y tilt demand
p 1935 2544 100 0 0 EGU:arcsecs
p 1935 2848 100 0 0 FDSC:y tilt demand
p 1935 2640 100 0 0 FTVL:DOUBLE
p 1935 2608 100 0 0 PREC:2
p 2176 2832 100 0 0 SCAN:.05 second
p 2080 3264 100 0 -1 name:$(top)yNetTiltDmd
use esirs 1984 3319 100 0 xTiltDemand
xform 0 2192 3472
p 1935 3216 100 0 0 DESC:x tilt demand
p 1935 2896 100 0 0 EGU:arcsecs
p 1935 3200 100 0 0 FDSC:x tilt demand
p 1935 2992 100 0 0 FTVL:DOUBLE
p 1935 2960 100 0 0 PREC:2
p 2176 3184 100 0 0 SCAN:.05 second
p 2096 3632 100 0 -1 name:$(top)xNetTiltDmd
use bc200tr 784 1384 -100 0 frame
xform 0 2464 2688
[comments]
