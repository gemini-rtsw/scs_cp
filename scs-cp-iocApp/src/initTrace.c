/* initTrace.c */

/*
 * An initHook routine to trace the iocInit() process.
 * Prints out the name of each state as it is reached.
 */

#include <stdio.h>
#include <stdlib.h>

#include "initHooks.h"
#include "epicsTime.h"
#include "epicsExport.h"
#include "iocsh.h"

static const char * timeFormatStr = "%Y-%m-%dT%H:%M:%S.%09f:%z";


static void trace(initHookState state) {
    static epicsTimeStamp ets;
    char timebuf[60];

    epicsTimeGetCurrent(&ets);
    epicsTimeToStrftime(timebuf, sizeof(timebuf), timeFormatStr, &ets);

    printf("iocInit: Trace@ %s:<%s>\n", initHookName(state), timebuf);
    
}

int traceIocInit(void) {
    static int done = 0;
    if (done)
        return -1;
    done = 1;

    initHookRegister(trace);
    puts("iocInit will be traced");
    return 0;
}


static const iocshFuncDef traceInitFuncDef = {"traceIocInit", 0, NULL};
static void traceInitFunc(const iocshArgBuf *args) {
    traceIocInit();
}

static void initTraceRegister(void) {
    iocshRegister(&traceInitFuncDef, traceInitFunc);
}
epicsExportRegistrar(initTraceRegister);

