[schematic2]
uniq 69
[tools]
[detail]
s 2256 3072 300 0 TWST - capability
s 2240 3168 300 0 ONST - port of event bus
s 2240 3280 300 0 ZRST - instrument name
s 2224 3456 300 0 instrument are stored in these records
s 2224 3536 300 0 The configuration and capabilities of each
s 3600 1664 100 0 Secondary Control System
s 3600 1600 100 0 Instrument configuration data
s 3376 1552 100 0 author: S.Prior
s 3360 1520 100 0 checked: S.Prior
s 3856 1536 100 0 1
s 3984 1536 100 0 1
s 3616 1552 100 0 15-Jan-98
s 3408 3824 100 0 instruments.sch
[cell use]
use embbos 1440 2055 100 0 ics4
xform 0 1568 2144
p 1376 2363 100 0 0 DESC:
p 1504 2016 100 0 1 PV:$(top)
use embbos 1440 2343 100 0 ics3
xform 0 1568 2432
p 1376 2651 100 0 0 DESC:
p 1504 2304 100 0 1 PV:$(top)
use embbos 1440 2599 100 0 ics2
xform 0 1568 2688
p 1376 2907 100 0 0 DESC:
p 1504 2560 100 0 1 PV:$(top)
use embbos 1440 2855 100 0 ics1
xform 0 1568 2944
p 1376 3163 100 0 0 DESC:
p 1504 2816 100 0 1 PV:$(top)
use embbos 1440 3143 100 0 ics0
xform 0 1568 3232
p 1376 3451 100 0 0 DESC:
p 1504 3104 100 0 1 PV:$(top)
use embbos 1440 3431 100 0 scs0
xform 0 1568 3520
p 1376 3739 100 0 0 DESC:
p 1504 3392 100 0 1 PV:$(top)
use bc200tr 784 1384 -100 0 frame
xform 0 2464 2688
[comments]
