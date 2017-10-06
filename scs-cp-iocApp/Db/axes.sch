[schematic2]
uniq 463
[tools]
[detail]
w 202 235 100 0 n#457 ebis.enable.VAL 80 224 384 224 egenSub.mechanism.INPA
w -1532 171 100 0 n#438 elongins.jog.FLNK -1600 384 -1536 384 -1536 -32 -1408 -32 esubs.eventSim.SLNK
w -1534 363 100 0 n#437 elongins.jog.VAL -1600 352 -1408 352 esubs.eventSim.INPA
s -240 400 100 0 read into X and Y tilt
s -224 448 100 0 by record driveChop and
s -240 496 100 0 the demands are calculated
s -240 544 100 0 for triangular chopping
s 528 720 100 0 axes.sch
s 1104 -1552 100 0 1
s 992 -1552 100 0 1
s 752 -1568 100 0 15-Mar-98
s 736 -1504 100 0 Tilt system simulation axes
s 480 -1584 100 0 checked:S.Prior
s 480 -1552 100 0 author:S.Prior
s 736 -1440 100 0 Secondary Control System
s -1792 608 100 0 setting the jog record to greater than zero
s -1792 576 100 0 causes the subroutine record to set a semaphore simulating
s -1792 544 100 0 a chop transition request from the event system
s -416 -48 100 0 chop configuration parameters are written to
s -416 -96 100 0 the inputs of the driveChop gensub from SNL in file tilt_st.st
[cell use]
use egenSub 384 -537 100 0 mechanism
xform 0 528 -112
p 227 203 100 0 0 DESC:tilt system axes
p 161 -763 100 0 0 FTA:LONG
p 161 -763 100 0 0 FTB:LONG
p 161 -795 100 0 0 FTC:LONG
p 161 -827 100 0 0 FTD:DOUBLE
p 161 -859 100 0 0 FTE:LONG
p 161 -923 100 0 0 FTF:DOUBLE
p 161 -923 100 0 0 FTG:DOUBLE
p 161 -763 100 0 0 FTVA:LONG
p 161 -763 100 0 0 FTVB:DOUBLE
p 161 -827 100 0 0 FTVD:DOUBLE
p 448 -624 100 0 1 INAM:
p 96 14 100 0 0 PREC:2
p 464 -576 100 0 1 PV:$(top)
p 448 -704 100 0 1 SCAN:.05 second
p 448 -656 100 0 1 SNAM:mechSim
use ebis -176 167 100 0 enable
xform 0 -48 240
p -349 315 100 0 0 DESC:tilt mechanism enable switch
p -400 78 100 0 0 ONAM:DISABLED
p -176 128 100 0 1 PV:$(top)
p -400 110 100 0 0 ZNAM:ENABLED
use ebis -800 327 100 0 ebis#459
xform 0 -672 400
p -973 475 100 0 0 DESC:coincidence string
p -800 240 100 0 1 ONAM:FALSE
p -800 304 100 0 1 PV:$(top)
p -800 272 100 0 1 ZNAM:TRUE
use bc200tr -2096 -1720 -100 0 frame
xform 0 -416 -416
use elongins -800 519 100 0 elongins#458
xform 0 -672 592
p -1011 793 100 0 0 DESC:tilt beam select index
p -800 496 100 0 1 PV:$(top)
use elongins -1792 272 100 0 jog
xform 0 -1728 368
p -1792 288 100 0 1 PV:$(top)
use esubs -1408 -121 100 0 eventSim
xform 0 -1264 144
p -1565 331 100 0 0 DESC:chop demand sim
p -1408 -176 100 0 1 INAM:stateInit
p -1344 -144 100 0 1 PV:$(top)
p -1376 -304 100 0 1 SCAN:Passive
p -1376 -256 100 0 1 SNAM:driveEvent
[comments]
