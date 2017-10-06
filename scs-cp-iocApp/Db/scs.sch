[schematic2]
uniq 112
[tools]
[detail]
s 160 880 200 0 The whole design makes extensive use of SNL for communication and control between the blocks
s 160 944 200 0 The tilt block contains all of the simulation functions for the tilt system
s 1840 1024 200 0 according to functionality
s 160 1024 200 0 The schematic shows the SCS broken down into a hierarchical structure
s 2592 3024 100 0 scs.sch
s 2816 752 100 0 22-Oct-97
s 2544 720 100 0 checked: S.Prior
s 2544 752 100 0 author: S.Prior
s 3152 736 100 0 1
s 3056 736 100 0 1
s 2784 800 100 0 SCS Top Schematic
s 2784 864 100 0 Secondary Control System
s 2416 2400 100 0 file: scs_st.st
s 2432 2336 100 0 ss CADprocessor
s 2432 2304 100 0 ss moveDemandProcessing
s 2432 2272 100 0 ss guideDemandProcessing
s 2432 2240 100 0 ss chopDemandProcessing
s 2432 2208 100 0 ss monitorProcess
s 2192 1824 200 0 Maintenance/Recovery Routines
[cell use]
use driveXY 2368 1031 100 0 driveXY#111
xform 0 2528 1216
use driveCB 1984 1031 100 0 driveCB#110
xform 0 2144 1216
use driveDB 2752 1383 100 0 driveDB#109
xform 0 2912 1584
use driveOffloader 2368 1383 100 0 driveOffloader#108
xform 0 2528 1584
use driveFollower 1984 1383 100 0 driveFollower#107
xform 0 2144 1584
use control 1024 1367 100 0 control#105
xform 0 1192 1576
p 1056 1712 100 0 1 seta:XTILT 0
p 1056 1664 100 0 1 setb:YTILT 1
use monitors 1536 1367 100 0 monitors#104
xform 0 1696 1576
use tilt 1392 2127 100 0 tilt#103
xform 0 1696 2408
p 1536 2208 100 0 1 seta:top $(top)tilt:
use house 1536 1783 100 0 house
xform 0 1696 1992
p 1552 2144 100 0 1 seta:group house:
use refmem 512 1367 100 0 refmem
xform 0 672 1576
p 528 1728 100 0 1 seta:group refmem:
use snl 2272 2007 100 0 snl#84
xform 0 2576 2336
use move 1024 1808 100 0 move
xform 0 1184 1992
p 1056 2144 100 0 1 seta:group move:
use guide 512 1792 100 0 guide
xform 0 672 2000
p 544 2144 100 0 1 seta:group guide:
use chop 512 2224 100 0 chop
xform 0 672 2408
p 544 2560 100 0 1 seta:group chop:
use test 1024 2208 100 0 test
xform 0 1184 2408
p 1056 2560 100 0 1 seta:group test:
use bc200tr -32 584 -100 0 frame
xform 0 1648 1888
[comments]
