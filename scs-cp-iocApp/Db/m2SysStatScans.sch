[schematic2]
uniq 68
[tools]
[detail]
w 1416 1890 100 0 n#67 eaos.eaos#62.OUT 1344 1888 1536 1888 hwout.hwout#59.outp
w 1416 1618 100 0 n#66 eaos.eaos#63.OUT 1344 1616 1536 1616 hwout.hwout#60.outp
w 1416 1346 100 0 n#65 eaos.eaos#64.OUT 1344 1344 1536 1344 hwout.hwout#61.outp
w 488 1346 100 0 n#52 eaos.eaos#53.OUT 416 1344 608 1344 hwout.hwout#56.outp
w 488 1618 100 0 n#51 eaos.eaos#54.OUT 416 1616 608 1616 hwout.hwout#57.outp
w 488 1890 100 0 n#50 eaos.eaos#55.OUT 416 1888 608 1888 hwout.hwout#58.outp
w 1416 2178 100 0 n#47 eaos.eaos#48.OUT 1344 2176 1536 2176 hwout.hwout#49.outp
w 2344 2178 100 0 n#23 eaos.eaos#21.OUT 2272 2176 2464 2176 hwout.hwout#22.outp
w 488 2178 100 0 n#17 eaos.eaos#15.OUT 416 2176 608 2176 hwout.hwout#16.outp
s 2432 2448 100 0 $Id: m2SysStatScans.sch,v 1.1 2002/02/20 12:41:58 ajf Exp $
[cell use]
use eaos 1112 2120 100 0 eaos#48
xform 0 1216 2208
p 1136 2078 100 0 1 DTYP:VX stats
p 1200 2112 100 1024 -1 name:$(top)fdScanPeriod
use eaos 2040 2120 100 0 eaos#21
xform 0 2144 2208
p 2064 2078 100 0 1 DTYP:VX stats
p 2128 2112 100 1024 -1 name:$(top)cpuScanPeriod
use eaos 184 2120 100 0 eaos#15
xform 0 288 2208
p 208 2078 100 0 1 DTYP:VX stats
p 272 2112 100 1024 -1 name:$(top)memoryScanPeriod
use eaos 184 1288 100 0 eaos#53
xform 0 288 1376
p 208 1246 100 0 1 DTYP:VX stats
p 272 1280 100 1024 -1 name:$(top)memoryTotalScanPeriod
use eaos 184 1560 100 0 eaos#54
xform 0 288 1648
p 208 1518 100 0 1 DTYP:VX stats
p 272 1552 100 1024 -1 name:$(top)memoryFreeScanPeriod
use eaos 184 1832 100 0 eaos#55
xform 0 288 1920
p 208 1790 100 0 1 DTYP:VX stats
p 272 1824 100 1024 -1 name:$(top)memoryUsedScanPeriod
use eaos 1112 1832 100 0 eaos#62
xform 0 1216 1920
p 1136 1790 100 0 1 DTYP:VX stats
p 1200 1824 100 1024 -1 name:$(top)fdUsedScanPeriod
use eaos 1112 1560 100 0 eaos#63
xform 0 1216 1648
p 1136 1518 100 0 1 DTYP:VX stats
p 1200 1552 100 1024 -1 name:$(top)fdFreeScanPeriod
use eaos 1112 1288 100 0 eaos#64
xform 0 1216 1376
p 1136 1246 100 0 1 DTYP:VX stats
p 1200 1280 100 1024 -1 name:$(top)fdTotalScanPeriod
use hwout 1560 2136 100 0 hwout#49
xform 0 1632 2176
p 1520 2222 100 0 -1 val(outp):@fdScanPeriod
use hwout 2488 2136 100 0 hwout#22
xform 0 2560 2176
p 2448 2222 100 0 -1 val(outp):@cpuScanPeriod
use hwout 632 2136 100 0 hwout#16
xform 0 704 2176
p 576 2222 100 0 -1 val(outp):@memoryScanPeriod
use hwout 632 1304 100 0 hwout#56
xform 0 704 1344
p 592 1390 100 0 -1 val(outp):@memoryTotalScanPeriod
use hwout 632 1576 100 0 hwout#57
xform 0 704 1616
p 592 1662 100 0 -1 val(outp):@memoryFreeScanPeriod
use hwout 632 1848 100 0 hwout#58
xform 0 704 1888
p 576 1934 100 0 -1 val(outp):@memoryUsedScanPeriod
use hwout 1560 1848 100 0 hwout#59
xform 0 1632 1888
p 1504 1934 100 0 -1 val(outp):@fdUsedScanPeriod
use hwout 1560 1576 100 0 hwout#60
xform 0 1632 1616
p 1520 1662 100 0 -1 val(outp):@fdFreeScanPeriod
use hwout 1560 1304 100 0 hwout#61
xform 0 1632 1344
p 1520 1390 100 0 -1 val(outp):@fdTotalScanPeriod
use bc200tr -144 -24 -100 0 frame
xform 0 1536 1280
[comments]
