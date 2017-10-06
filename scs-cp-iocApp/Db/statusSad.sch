[schematic2]
uniq 68
[tools]
[detail]
w 1224 3243 100 0 n#65 hwin.hwin#64.in 1248 3232 1248 3232 esirs.health.INP
w 2408 2859 100 0 n#59 hwin.hwin#58.in 2432 2848 2432 2848 esirs.topend.INP
s 3408 3824 100 0 statusSad.sch
s 3600 1664 100 0 Secondary Control System
s 3600 1600 100 0 SCS status SIRs
s 3376 1552 100 0 author: S.Prior
s 3360 1520 100 0 checked: S.Prior
s 3856 1536 100 0 1
s 3984 1536 100 0 1
s 3616 1552 100 0 27-Apr-98
[cell use]
use hwin 2240 2807 100 0 hwin#58
xform 0 2336 2848
p 2096 2800 100 0 1 val(in):$(top)top.VAL
use hwin 1056 3191 100 0 hwin#64
xform 0 1152 3232
p 912 3264 100 0 -1 val(in):$(top)scanHealth.VALA
use esirs 1248 2631 100 0 inposition
xform 0 1456 2784
p 1199 2528 100 0 0 DESC:scs in position
p 1199 2208 100 0 0 EGU:none
p 1199 2304 100 0 0 FTVL:STRING
p 1360 2928 100 0 -1 name:$(top)inPosition
use esirs 1248 2983 100 0 health
xform 0 1456 3136
p 1199 2880 100 0 0 DESC:scs health
p 1199 2560 100 0 0 EGU:none
p 1632 3104 100 0 1 FTVL:STRING
p 1648 3072 100 0 1 SCAN:Passive
p 1376 3280 100 0 -1 name:$(top)health
p 1200 3232 75 1024 -1 pproc(INP):NPP
use esirs 1248 3335 100 0 state
xform 0 1456 3488
p 1199 3232 100 0 0 DESC:scs state
p 1199 2912 100 0 0 EGU:none
p 1184 3040 100 0 0 FTVL:LONG
p 1199 2944 100 0 0 SNAM:
p 1392 3632 100 0 -1 name:$(top)state
use esirs 2432 2983 100 0 deployable
xform 0 2640 3136
p 2383 2560 100 0 0 EGU:none
p 2512 3296 100 0 -1 name:$(top)CentralBafflePos
use esirs 2432 3335 100 0 central
xform 0 2640 3488
p 2383 2912 100 0 0 EGU:none
p 2528 3632 100 0 -1 name:$(top)DeployBafflePos
use esirs 2432 2599 100 0 topend
xform 0 2640 2752
p 2383 2176 100 0 0 EGU:none
p 2383 2272 100 0 0 FTVL:STRING
p 2624 2464 100 0 0 SCAN:1 second
p 2544 2912 100 0 -1 name:$(top)topEnd
use esirs 1248 2215 100 0 esirs#53
xform 0 1456 2368
p 1360 2528 100 0 -1 name:$(top)simulation
p 1200 2464 75 1024 -1 pproc(INP):NPP
use esirs 2432 2215 100 0 esirs#54
xform 0 2640 2368
p 2383 1888 100 0 0 FTVL:DOUBLE
p 2640 2528 100 256 -1 name:$(top)temperature
use esirs 3264 3335 100 0 chopping
xform 0 3472 3488
p 3328 3648 100 0 1 PV:$(top)
use bc200tr 784 1384 -100 0 frame
xform 0 2464 2688
[comments]
