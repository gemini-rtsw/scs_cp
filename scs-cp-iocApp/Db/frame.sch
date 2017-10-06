[schematic2]
uniq 132
[tools]
[detail]
s 2784 864 100 0 Secondary Control System
s 2768 800 100 0 frame of reference conversion records
s 3056 736 100 0 1
s 3152 736 100 0 1
s 2544 752 100 0 author: S.Prior
s 2544 720 100 0 checked: S.Prior
s 2816 752 100 0 11-Nov-97
s 2592 3024 100 0 frame.sch
s 304 1616 300 0 These records hold the coordinate conversion
s 320 1552 300 0 angles and offsets to convert from
s 336 1488 300 0 TCS and instrument frames of reference to m2
[cell use]
use eais 2464 2535 100 0 focusScaling
xform 0 2592 2608
p 2253 2681 100 0 0 DESC:
p 2208 2350 100 0 0 EGU:degrees
p 2208 2446 100 0 0 PREC:2
p 2528 2496 100 0 1 PV:$(top)
p 2528 2464 100 0 1 SCAN:Passive
use eais 640 2535 100 0 tiltSkew
xform 0 768 2608
p 429 2681 100 0 0 DESC:
p 384 2350 100 0 0 EGU:degrees
p 384 2446 100 0 0 PREC:2
p 704 2496 100 0 1 PV:$(top)
p 704 2464 100 0 1 SCAN:Passive
use eais 640 2247 100 0 tiltOffsetX
xform 0 768 2320
p 429 2393 100 0 0 DESC:
p 384 2062 100 0 0 EGU:microns
p 384 2158 100 0 0 PREC:2
p 704 2208 100 0 1 PV:$(top)
p 704 2176 100 0 1 SCAN:Passive
use eais 640 1927 100 0 tiltOffsetY
xform 0 768 2000
p 429 2073 100 0 0 DESC:
p 384 1742 100 0 0 EGU:microns
p 384 1838 100 0 0 PREC:2
p 704 1888 100 0 1 PV:$(top)
p 704 1856 100 0 1 SCAN:Passive
use eais 1088 1927 100 0 posOffsetY
xform 0 1216 2000
p 877 2073 100 0 0 DESC:
p 832 1742 100 0 0 EGU:microns
p 832 1838 100 0 0 PREC:2
p 1152 1888 100 0 1 PV:$(top)
p 1152 1856 100 0 1 SCAN:Passive
use eais 1088 2247 100 0 posOffsetX
xform 0 1216 2320
p 877 2393 100 0 0 DESC:
p 832 2062 100 0 0 EGU:microns
p 832 2158 100 0 0 PREC:2
p 1152 2208 100 0 1 PV:$(top)
p 1152 2176 100 0 1 SCAN:Passive
use eais 1088 2535 100 0 posSkew
xform 0 1216 2608
p 877 2681 100 0 0 DESC:
p 832 2350 100 0 0 EGU:degrees
p 832 2446 100 0 0 PREC:2
p 1152 2496 100 0 1 PV:$(top)
p 1152 2464 100 0 1 SCAN:Passive
use eais 1536 1927 100 0 gaosOffsetY
xform 0 1664 2000
p 1325 2073 100 0 0 DESC:
p 1280 1742 100 0 0 EGU:microns
p 1280 1838 100 0 0 PREC:2
p 1600 1888 100 0 1 PV:$(top)
p 1600 1856 100 0 1 SCAN:Passive
use eais 1536 2247 100 0 gaosOffsetX
xform 0 1664 2320
p 1325 2393 100 0 0 DESC:
p 1280 2062 100 0 0 EGU:microns
p 1280 2158 100 0 0 PREC:2
p 1600 2208 100 0 1 PV:$(top)
p 1600 2176 100 0 1 SCAN:Passive
use eais 1536 2535 100 0 gaosSkew
xform 0 1664 2608
p 1325 2681 100 0 0 DESC:
p 1280 2350 100 0 0 EGU:degrees
p 1280 2446 100 0 0 PREC:2
p 1600 2496 100 0 1 PV:$(top)
p 1600 2464 100 0 1 SCAN:Passive
use eais 1984 2535 100 0 gyroSkew
xform 0 2112 2608
p 1773 2681 100 0 0 DESC:
p 1728 2350 100 0 0 EGU:degrees
p 1728 2446 100 0 0 PREC:2
p 2048 2496 100 0 1 PV:$(top)
p 2048 2464 100 0 1 SCAN:Passive
use eais 1984 2247 100 0 gyroOffsetX
xform 0 2112 2320
p 1773 2393 100 0 0 DESC:
p 1728 2062 100 0 0 EGU:microns
p 1728 2158 100 0 0 PREC:2
p 2048 2208 100 0 1 PV:$(top)
p 2048 2176 100 0 1 SCAN:Passive
use eais 1984 1927 100 0 gyroOffsetY
xform 0 2112 2000
p 1773 2073 100 0 0 DESC:
p 1728 1742 100 0 0 EGU:microns
p 1728 1838 100 0 0 PREC:2
p 2048 1888 100 0 1 PV:$(top)
p 2048 1856 100 0 1 SCAN:Passive
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
