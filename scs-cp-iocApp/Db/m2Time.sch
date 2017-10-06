[schematic2]
uniq 4
[tools]
[detail]
s 960 1920 200 0 These records are needed for the timeSeq SNL code
s 2816 80 100 0 Records for timeseq 
s 2592 2272 100 0 $Id: m2Time.sch,v 1.2 2000/10/21 00:58:00 dayle Exp $
[cell use]
use esirs 1184 1287 100 0 health
xform 0 1392 1440
p 1248 1600 100 0 -1 DESC:Time system health
p 1312 1248 100 1024 1 name:$(top)TIME:$(I)
use ebos 1208 584 100 0 intSimulate
xform 0 1312 672
p 1216 782 100 0 -1 DESC:Time simulation flag
p 864 526 100 0 0 ONAM:True
p 1088 654 100 0 0 OSV:MINOR
p 864 558 100 0 0 ZNAM:False
p 1296 528 100 1024 1 name:$(top)TIME:$(I)
use estringins 1208 968 100 0 logrecord
xform 0 1312 1040
p 1232 1118 100 0 -1 DESC:DHS log record
p 1264 928 100 1024 1 name:$(top)$(I)
use bc200tr 0 -200 -100 0 frame
xform 0 1680 1104
[comments]
