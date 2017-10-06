[schematic2]
uniq 59
[tools]
[detail]
w 3016 2859 100 0 n#58 hwin.hwin#57.in 3040 2848 3040 2848 esirs.zFocusGuide.INP
w 3016 3243 100 0 n#56 hwin.hwin#55.in 3040 3232 3040 3232 esirs.yTiltGuide.INP
w 3016 3595 100 0 n#54 hwin.hwin#53.in 3040 3584 3040 3584 esirs.xTiltGuide.INP
s 3600 1664 100 0 Secondary Control System
s 3600 1600 100 0 SCS SIR Records
s 3376 1552 100 0 author: S.Prior
s 3360 1520 100 0 checked: S.Prior
s 3856 1536 100 0 1
s 3984 1536 100 0 1
s 3616 1552 100 0 26-Aug-96
s 3408 3824 100 0 guideSad.sch
[cell use]
use hwin 2848 3543 100 0 hwin#53
xform 0 2944 3584
p 2656 3520 100 0 1 val(in):$(top)decimator.VALD
use hwin 2848 3191 100 0 hwin#55
xform 0 2944 3232
p 2656 3168 100 0 1 val(in):$(top)decimator.VALE
use hwin 2848 2807 100 0 hwin#57
xform 0 2944 2848
p 2656 2784 100 0 1 val(in):$(top)decimator.VALF
use esirs 3040 3335 100 0 xTiltGuide
xform 0 3248 3488
p 2991 2912 100 0 0 EGU:arcsecs
p 2991 3008 100 0 0 FTVL:DOUBLE
p 2991 2976 100 0 0 PREC:2
p 3232 3200 100 0 0 SCAN:.05 second
p 3168 3648 100 0 -1 name:$(top)xTiltGuide
use esirs 3040 2983 100 0 yTiltGuide
xform 0 3248 3136
p 2991 2560 100 0 0 EGU:arcsecs
p 2991 2656 100 0 0 FTVL:DOUBLE
p 2991 2624 100 0 0 PREC:2
p 3232 2848 100 0 0 SCAN:.05 second
p 3168 3280 100 0 -1 name:$(top)yTiltGuide
use esirs 3040 2599 100 0 zFocusGuide
xform 0 3248 2752
p 2991 1760 100 0 0 DISS:NO_ALARM
p 2991 2304 100 0 0 DISV:1
p 2991 2176 100 0 0 EGU:units
p 2991 2272 100 0 0 FTVL:DOUBLE
p 2991 2240 100 0 0 PREC:2
p 3232 2464 100 0 0 SCAN:.05 second
p 3168 2896 100 0 -1 name:$(top)zGuide
use bc200tr 784 1384 -100 0 frame
xform 0 2464 2688
[comments]
