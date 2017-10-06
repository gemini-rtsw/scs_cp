[schematic2]
uniq 420
[tools]
[detail]
f -3840 -304 -3264 -208 -100 256 decstext
s -2656 -1456 100 0 Secondary Control System
s -2656 -1520 100 0 DECS Control Parameters
s -2896 -1568 100 0 author: S. Prior
s -2896 -1600 100 0 checked: S. Prior
s -2640 -1568 100 0 20-Jun-97
s -2400 -1584 100 0 1
s -2272 -1584 100 0 1
s -3808 -240 100 0 This schematic only contains records acting
s -3808 -272 100 0 as sinks for some DECS control parameters
s -2848 704 100 0 decs.sch
[cell use]
use ebis -4384 -761 100 0 decsreset
xform 0 -4256 -688
p -4608 -850 100 0 0 ONAM:ON
p -4384 -800 100 0 1 PV:$(top)$(sys)
p -4608 -818 100 0 0 ZNAM:OFF
use ebis -4384 -569 100 0 decsfreeze
xform 0 -4256 -496
p -4608 -658 100 0 0 ONAM:ON
p -4384 -608 100 0 1 PV:$(top)$(sys)
p -4608 -626 100 0 0 ZNAM:OFF
use ebis -4384 -377 100 0 decspause
xform 0 -4256 -304
p -4608 -466 100 0 0 ONAM:ON
p -4384 -400 100 0 1 PV:$(top)$(sys)
p -4608 -434 100 0 0 ZNAM:OFF
use ebis -4384 -185 100 0 decson
xform 0 -4256 -112
p -4608 -274 100 0 0 ONAM:ON
p -4384 -208 100 0 1 PV:$(top)$(sys)
p -4608 -242 100 0 0 ZNAM:OFF
use bc200tr -5472 -1736 -100 0 frame
xform 0 -3792 -432
[comments]
