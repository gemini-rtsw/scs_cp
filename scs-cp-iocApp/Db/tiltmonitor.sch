[schematic2]
uniq 496
[tools]
[detail]
w -2446 331 100 0 n#361 ecalcs.heartbeat.VAL -2240 64 -2176 64 -2176 320 -2656 320 -2656 256 -2528 256 ecalcs.heartbeat.INPA
s -496 -704 100 0 Secondary Control System
s -752 -816 100 0 author:S.Prior
s -752 -848 100 0 checked:S.Prior
s -496 -768 100 0 tilt system simulation monitors
s -480 -832 100 0 02-Feb-98
s -240 -816 100 0 1
s -128 -816 100 0 1
s -2336 -208 100 0 CALC: A+1
s -704 1456 100 0 tiltmonitor.sch
[cell use]
use ebis -1696 647 -100 0 space
xform 0 -1568 720
p -1920 558 100 0 0 ONAM:ACTUATOR
p -1920 590 100 0 0 ZNAM:TILT
p -1520 640 100 1024 1 name:$(top)space
use ebis -1664 871 -100 0 vibControl
xform 0 -1536 944
p -1888 782 100 0 0 ONAM:DISABLED
p -1888 814 100 0 0 ZNAM:ENABLED
p -1536 848 100 1024 1 name:$(top)vibControl
use ebis -1664 1127 -100 0 offLoaders
xform 0 -1536 1200
p -1837 1275 100 0 0 DESC:offloader enable switch
p -1888 1038 100 0 0 ONAM:DISABLED
p -1888 1070 100 0 0 ZNAM:ENABLED
p -1536 1104 100 1024 1 name:$(top)offLoaders
use ebis -2080 663 -100 0 topendstatus
xform 0 -1952 736
p -2253 811 100 0 0 DESC:top end status
p -2304 574 100 0 0 ONAM:F6
p -1968 734 100 0 -1 Type:bi
p -2304 606 100 0 0 ZNAM:F16
p -1936 624 100 1024 1 name:$(top)topendstatus
use ebis -1696 103 100 0 ebis#491
xform 0 -1568 176
p -1869 251 100 0 0 DESC:error status
p -1920 14 100 0 0 ONAM:ERROR
p -1920 46 100 0 0 ZNAM:OK
p -1568 64 100 1024 1 name:$(top)actuatorLimit
use ebis -1696 -89 100 0 ebis#492
xform 0 -1568 -16
p -1869 59 100 0 0 DESC:error status
p -1920 -178 100 0 0 ONAM:ERROR
p -1920 -146 100 0 0 ZNAM:OK
p -1568 -128 100 1024 1 name:$(top)thermalLimit
use ebis -1696 -313 100 0 ebis#493
xform 0 -1568 -240
p -1869 -165 100 0 0 DESC:error status
p -1920 -402 100 0 0 ONAM:ERROR
p -1920 -370 100 0 0 ZNAM:OK
p -1568 -352 100 1024 1 name:$(top)mirrorDsp
use ebis -1696 -537 100 0 ebis#494
xform 0 -1568 -464
p -1869 -389 100 0 0 DESC:error status
p -1920 -626 100 0 0 ONAM:ERROR
p -1920 -594 100 0 0 ZNAM:OK
p -1568 -576 100 1024 1 name:$(top)vibrationDsp
use ebis -1696 295 100 0 ebis#495
xform 0 -1568 368
p -1869 443 100 0 0 DESC:error status
p -1920 206 100 0 0 ONAM:ERROR
p -1920 238 100 0 0 ZNAM:OK
p -1568 256 100 1024 1 name:$(top)sensorLimit
use eais -2528 1127 -100 0 temperature
xform 0 -2400 1200
p -2739 1273 100 0 0 DESC:enclosure temperature
p -2784 942 100 0 0 EGU:degrees celcius
p -2784 1038 100 0 0 PREC:2
p -2400 1104 100 1024 1 name:$(top)temperature
use eais -1280 1127 -100 0 bandwidth
xform 0 -1152 1200
p -1536 942 100 0 0 EGU:Hz
p -1536 1102 100 0 0 PINI:NO
p -1168 1104 100 1024 1 name:$(top)bandwidth
use estringins -1280 871 -100 0 currentBeam
xform 0 -1152 944
p -1152 832 100 1024 1 name:$(top)currentBeam
use estringins -2080 871 -100 0 tilthealth
xform 0 -1952 944
p -1952 832 100 1024 1 name:$(top)tilthealth
use estringins -2080 1127 -100 0 tiltstate
xform 0 -1952 1200
p -1952 1088 100 1024 1 name:$(top)tiltstate
use embbos -2528 583 -100 0 central
xform 0 -2400 672
p -2592 891 100 0 0 DESC:central baffle
p -2368 734 100 0 0 ONST:OPEN
p -2368 766 100 0 0 ZRST:CLOSED
p -2416 544 100 1024 1 name:$(top)central
use embbos -2528 839 -100 0 deployable
xform 0 -2400 928
p -2592 1147 100 0 0 DESC:deployable baffle
p -2368 990 100 0 0 ONST:NEAR IR
p -2368 926 100 0 0 THST:EXTENDED
p -2368 958 100 0 0 TWST:VISIBLE
p -2368 1022 100 0 0 ZRST:RETRACTED
p -2400 800 100 1024 1 name:$(top)deployable
use bc200tr -3328 -984 -100 0 frame
xform 0 -1648 320
use ecalcs -2528 -217 100 0 heartbeat
xform 0 -2384 48
p -2621 264 100 0 0 CALC:A+1
p -2528 -240 100 0 1 PV:$(top)
p -2528 -256 100 0 1 SCAN:1 second
[comments]
