[schematic2]
uniq 83
[tools]
[detail]
w 2960 1563 100 0 n#82 esirs.yError.FLNK 3664 2192 3760 2192 3760 1904 3312 1904 3312 1552 2656 1552 2656 1744 2864 1744 ecalcouts.xyerrMag.SLNK
w 2984 1643 100 0 n#81 esirs.yError.VAL 3664 2160 3712 2160 3712 1936 3264 1936 3264 1632 2752 1632 2752 1808 2864 1808 ecalcouts.xyerrMag.INPB
w 3208 2283 100 0 n#80 esirs.xError.VAL 3664 2512 3712 2512 3712 2272 2752 2272 2752 1840 2864 1840 ecalcouts.xyerrMag.INPA
w 3224 2203 100 0 n#77 hwin.hwin#76.in 3248 2192 3248 2192 esirs.yError.INP
w 3224 2555 100 0 n#75 hwin.hwin#74.in 3248 2544 3248 2544 esirs.xError.INP
w 1160 2203 100 0 n#73 hwin.hwin#72.in 1184 2192 1184 2192 esirs.yPosition.INP
w 1160 2555 100 0 n#71 hwin.hwin#70.in 1184 2544 1184 2544 esirs.xPosition.INP
w 3224 2891 100 0 n#69 hwin.hwin#68.in 3248 2880 3248 2880 esirs.zFocusError.INP
w 3224 3275 100 0 n#67 hwin.hwin#66.in 3248 3264 3248 3264 esirs.yTiltError.INP
w 3224 3627 100 0 n#65 hwin.hwin#64.in 3248 3616 3248 3616 esirs.xTiltError.INP
w 1160 2891 100 0 n#63 hwin.hwin#62.in 1184 2880 1184 2880 esirs.zFocusPos.INP
w 1160 3627 100 0 n#61 hwin.hwin#60.in 1184 3616 1184 3616 esirs.xTiltPos.INP
w 1160 3275 100 0 n#59 hwin.hwin#58.in 1184 3264 1184 3264 esirs.yTiltPos.INP
s 3600 1664 100 0 Secondary Control System
s 3600 1600 100 0 SCS position SIRs
s 3376 1552 100 0 author: S.Prior
s 3360 1520 100 0 checked: S.Prior
s 3856 1536 100 0 1
s 3984 1536 100 0 1
s 3616 1552 100 0 23-Dec-97
s 3408 3824 100 0 positionSad.sch
[cell use]
use ecalcouts 2864 1655 100 0 xyerrMag
xform 0 3024 1776
p 2928 1584 100 0 1 CALC:SQR(A^2+B^2)
p 2952 1888 100 0 0 SCAN:Passive
p 2976 1904 100 0 -1 name:$(top)xyerrMag
use hwin 3056 2151 100 0 hwin#76
xform 0 3152 2192
p 2960 2144 100 0 -1 val(in):$(top)decimator.VALR
use hwin 3056 2503 100 0 hwin#74
xform 0 3152 2544
p 2960 2496 100 0 -1 val(in):$(top)decimator.VALQ
use hwin 992 2151 100 0 hwin#72
xform 0 1088 2192
p 896 2144 100 0 -1 val(in):$(top)decimator.VALP
use hwin 992 2503 100 0 hwin#70
xform 0 1088 2544
p 896 2496 100 0 -1 val(in):$(top)decimator.VALO
use hwin 3056 2839 100 0 hwin#68
xform 0 3152 2880
p 2960 2832 100 0 -1 val(in):$(top)decimator.VALL
use hwin 3056 3223 100 0 hwin#66
xform 0 3152 3264
p 2960 3216 100 0 -1 val(in):$(top)decimator.VALK
use hwin 3056 3575 100 0 hwin#64
xform 0 3152 3616
p 2960 3568 100 0 -1 val(in):$(top)decimator.VALJ
use hwin 992 2839 100 0 hwin#62
xform 0 1088 2880
p 896 2832 100 0 -1 val(in):$(top)decimator.VALC
use hwin 992 3575 100 0 hwin#60
xform 0 1088 3616
p 896 3568 100 0 -1 val(in):$(top)decimator.VALA
use hwin 992 3223 100 0 hwin#58
xform 0 1088 3264
p 896 3216 100 0 -1 val(in):$(top)decimator.VALB
use esirs 2208 3367 100 0 esirs#57
xform 0 2416 3520
p 2368 3664 100 0 -1 name:$(top)Xc
use esirs 2208 3015 100 0 esirs#56
xform 0 2416 3168
p 2352 3312 100 0 -1 name:$(top)Yc
use esirs 2208 2631 100 0 esirs#55
xform 0 2416 2784
p 2368 2928 100 0 -1 name:$(top)Zc
use esirs 2208 2295 100 0 esirs#54
xform 0 2416 2448
p 2352 2592 100 0 -1 name:$(top)theta
use esirs 2208 1943 100 0 esirs#53
xform 0 2416 2096
p 2368 2240 100 0 -1 name:$(top)phi
use esirs 1184 3367 100 0 xTiltPos
xform 0 1392 3520
p 1135 3264 100 0 0 DESC:X tilt position
p 1135 2944 100 0 0 EGU:arcsecs
p 1135 3248 100 0 0 FDSC:x tilt position
p 1135 3040 100 0 0 FTVL:DOUBLE
p 1135 3008 100 0 0 PREC:2
p 1376 3232 100 0 0 SCAN:.05 second
p 1312 3664 100 0 -1 name:$(top)xTiltPos
use esirs 3248 3367 100 0 xTiltError
xform 0 3456 3520
p 3199 3264 100 0 0 DESC:X tilt error
p 3199 2944 100 0 0 EGU:arcsecs
p 3199 3248 100 0 0 FDSC:x tilt error
p 3199 3040 100 0 0 FTVL:DOUBLE
p 3199 3008 100 0 0 PREC:2
p 3440 3232 100 0 0 SCAN:.05 second
p 3376 3680 100 0 -1 name:$(top)xTiltErr
use esirs 1184 3015 100 0 yTiltPos
xform 0 1392 3168
p 1135 2912 100 0 0 DESC:Y tilt position
p 1135 2592 100 0 0 EGU:arcsecs
p 1135 2896 100 0 0 FDSC:y tilt position
p 1135 2688 100 0 0 FTVL:DOUBLE
p 1135 2656 100 0 0 PREC:2
p 1376 2880 100 0 0 SCAN:.05 second
p 1296 3312 100 0 -1 name:$(top)yTiltPos
use esirs 3248 3015 100 0 yTiltError
xform 0 3456 3168
p 3199 2912 100 0 0 DESC:y tilt error
p 3199 2592 100 0 0 EGU:arcsecs
p 3199 2896 100 0 0 FDSC:y tilt error
p 3199 2688 100 0 0 FTVL:DOUBLE
p 3199 2656 100 0 0 PREC:2
p 3440 2880 100 0 0 SCAN:.05 second
p 3360 3328 100 0 -1 name:$(top)yTiltErr
use esirs 3248 2631 100 0 zFocusError
xform 0 3456 2784
p 3199 2528 100 0 0 DESC:z focus error
p 3199 2208 100 0 0 EGU:microns
p 3199 2512 100 0 0 FDSC:z focus error
p 3199 2304 100 0 0 FTVL:DOUBLE
p 3199 2272 100 0 0 PREC:2
p 3440 2496 100 0 0 SCAN:.05 second
p 3392 2944 100 0 -1 name:$(top)zErr
use esirs 1184 2631 100 0 zFocusPos
xform 0 1392 2784
p 1135 2528 100 0 0 DESC:z focus position
p 1135 2208 100 0 0 EGU:microns
p 1135 2512 100 0 0 FDSC:z focus position
p 1135 2304 100 0 0 FTVL:DOUBLE
p 1135 2272 100 0 0 PREC:2
p 1376 2496 100 0 0 SCAN:.05 second
p 1328 2944 100 0 -1 name:$(top)zPos
use esirs 1184 2295 100 0 xPosition
xform 0 1392 2448
p 1135 1872 100 0 0 EGU:microns
p 1135 1968 100 0 0 FTVL:DOUBLE
p 1135 1936 100 0 0 PREC:2
p 1376 2160 100 0 0 SCAN:1 second
p 1312 2608 100 0 -1 name:$(top)xPos
use esirs 3248 2295 100 0 xError
xform 0 3456 2448
p 3199 1872 100 0 0 EGU:microns
p 3199 1968 100 0 0 FTVL:DOUBLE
p 3199 1936 100 0 0 PREC:2
p 3440 2160 100 0 0 SCAN:1 second
p 3376 2592 100 0 -1 name:$(top)xPosErr
use esirs 1184 1943 100 0 yPosition
xform 0 1392 2096
p 1135 1520 100 0 0 EGU:microns
p 1135 1616 100 0 0 FTVL:DOUBLE
p 1135 1584 100 0 0 PREC:2
p 1376 1808 100 0 0 SCAN:1 second
p 1328 2240 100 0 -1 name:$(top)yPos
use esirs 3248 1943 100 0 yError
xform 0 3456 2096
p 3199 1520 100 0 0 EGU:microns
p 3199 1616 100 0 0 FTVL:DOUBLE
p 3199 1584 100 0 0 PREC:2
p 3440 1808 100 0 0 SCAN:1 second
p 3376 2256 100 0 -1 name:$(top)yPosErr
use bc200tr 784 1384 -100 0 frame
xform 0 2464 2688
[comments]
