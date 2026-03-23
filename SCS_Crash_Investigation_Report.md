# SCS Crash Investigation Report
## Exception Vector 3/4 Memory Corruption Analysis

**System:** Gemini South Secondary Control System (SCS)
**Branch:** bugfix/isr-corruption-detection-2024q3
**Based on:** unstable/2024q3
**Issue:** [GSFR-43648](https://noirlab.atlassian.net/browse/GSFR-43648) | [GE7-94](https://noirlab.atlassian.net/browse/GE7-94)
**Author:** Patrick Parks
**Date:** December 2025

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Problem Description](#problem-description)
3. [Root Cause Investigation](#root-cause-investigation)
4. [Defensive Measures Implemented](#defensive-measures-implemented)
5. [EPICS Monitoring](#epics-monitoring)
6. [Next Steps](#next-steps)
7. [References](#references)

---

## Executive Summary

The SCS experiences intermittent crashes every 3-5 weeks caused by memory corruption of event pointers used in interrupt service routines. The system manifests as **"SCS froze and rebooted itself"** - interrupts are disabled when the crash occurs in ISR context, system hangs, then watchdog triggers reboot.

**Status:**
- ✅ **Vector 4 (Hardware)** - Fixed by XYCOM-240 board replacement (Aug 2025)
- ❌ **Vector 3 (Memory corruption)** - Still occurring, root cause unknown

**This Branch:** Implements two-layer defensive system to prevent crashes and collect diagnostic data while root cause is investigated.

---

## Problem Description

### Crash Symptoms

When operators report "SCS froze and rebooted itself":
1. Guide loops fail to re-close after telescope offset
2. SCS shows error state, all controls frozen
3. Watchdog timeout triggers automatic reboot
4. Occurs every 3-5 weeks, often during guide offset operations

### Technical Cause

VMI5588 reflective memory ISRs (`rmISR2`, `rmISR3`) crash when calling `epicsEventSignal()` on corrupted event pointers:

```
VME Interrupt → rmISR3() → epicsEventSignal(guideUpdateNow) → CRASH
```

Corrupted pointer values found in crash dumps:
- `NULL` (0x00000000)
- `0xa5a5a5a5` (RTEMS freed memory pattern)

### Affected Pointers

Two global event pointers used to signal ISRs:
- `scsReceiveNow` - signaled by `rmISR2()`
- `guideUpdateNow` - signaled by `rmISR3()`

These pointers are:
- Created once at startup
- Never destroyed or reassigned
- **Yet become corrupted after weeks of operation**

---

## Root Cause Investigation

### Code Analysis - All Clean

Extensive static code analysis found no definitive cause:

| Investigation | Result |
|--------------|--------|
| Buffer overflows | ✅ All buffers properly sized |
| Array bounds | ✅ All indices validated |
| Stack overflow | ✅ Large arrays are globals |
| VMI5588 driver | ✅ Function pointers handled correctly |
| Pointer arithmetic | ✅ No unsafe operations |

### Hypotheses

Since no code defects found, corruption likely caused by:

1. **Hardware Issues** - VME bus noise, memory bit flips, reflective memory card failure
2. **External Writes** - Another VME node writing to wrong memory location
3. **RTEMS Kernel Bug** - Memory management issue under specific conditions
4. **Undetected Software Bug** - Very subtle corruption in unreviewed code

**Conclusion:** Root cause remains unknown. Defensive measures implemented to survive corruption and collect evidence.

---

## Defensive Measures Implemented

This branch implements a two-layer defensive system:

### Layer 1: ISR Pointer Validation

**What:** Validate event pointers before use in ISR context
**Where:** `rmISR2()` and `rmISR3()` in `control.c`
**How:** Check for NULL and freed memory pattern (0xa5a5a5xx)

```c
void rmISR3(int node) {
    if (guideUpdateNow == NULL) {
        isr3_null_count++;      // Log corruption
        return;                  // Don't crash
    }
    if (IS_FREED_MEMORY_PATTERN(guideUpdateNow)) {
        isr3_freed_count++;     // Log corruption
        return;                  // Don't crash
    }
    epicsEventSignal(guideUpdateNow);  // Safe to use
}
```

**Result:** System continues operating (degraded guiding) instead of crashing

### Layer 2: Canary Corruption Detection

**What:** Sentinel values placed in memory around event pointers
**Where:** Global variables in `control.c`
**How:** Monitoring task checks every 1 second if canaries are overwritten

```c
// Memory layout:
unsigned long canary_before_scsReceiveNow = 0xDEADBEEF;  // Guard
epicsEventId scsReceiveNow = NULL;                        // Protected
unsigned long canary_after_scsReceiveNow = 0xCAFEBABE;   // Guard

unsigned long canary_before_guideUpdateNow = 0xFEEDFACE; // Guard
epicsEventId guideUpdateNow = NULL;                       // Protected
unsigned long canary_after_guideUpdateNow = 0xC0FFEE00;  // Guard
```

Monitoring task (`canaryCheckTask`) runs every 1 second:
```c
if (canary_before_scsReceiveNow != 0xDEADBEEF) scs_canary_before_corrupt++;
if (canary_after_scsReceiveNow != 0xCAFEBABE) scs_canary_after_corrupt++;
// ... check all 4 canaries
```

**Why DEADBEEF/CAFEBABE?**
- Human-readable in hex dumps for debugging
- Statistically unlikely to occur naturally (1 in 4 billion)
- Industry-standard magic numbers (Java uses 0xCAFEBABE, Microsoft uses 0xDEADBEEF)
- Invalid as memory addresses on most systems

**Advantages:**
- Detects corruption within 1 second (vs waiting for ISR to fire)
- Provides directional information (corruption from before vs after pointer)
- Catches transient corruption between ISR firings
- Memory layout verification ensures canaries are adjacent to protected pointers

### Comparison

| Feature | ISR Validation | Canary Detection |
|---------|----------------|------------------|
| **Detection timing** | Only when ISR fires | Every 1 second |
| **Directional info** | No | Yes (before/after) |
| **Transient corruption** | May miss | Catches all |
| **System impact** | Prevents crash | Prevents crash + forensics |

**Both layers work together:** ISR checks prevent crashes, canaries provide early warning and diagnostic information.

---

## EPICS Monitoring

### Available PVs (assuming SCSTOP=m2)

**ISR Corruption Counters:**
| PV | Description | Alarm |
|----|-------------|-------|
| `m2:ISR2_NULL_CNT` | NULL detections in rmISR2 | MAJOR if > 0 |
| `m2:ISR2_FREED_CNT` | Freed pattern detections in rmISR2 | MAJOR if > 0 |
| `m2:ISR3_NULL_CNT` | NULL detections in rmISR3 | MAJOR if > 0 |
| `m2:ISR3_FREED_CNT` | Freed pattern detections in rmISR3 | MAJOR if > 0 |

**Canary Corruption Counters:**
| PV | Description | Alarm |
|----|-------------|-------|
| `m2:CANARIES_ADJACENT` | Memory layout verified | MAJOR if not adjacent |
| `m2:SCS_CANARY_BEFORE_CORRUPT` | Corruption before scsReceiveNow | MAJOR if > 0 |
| `m2:SCS_CANARY_AFTER_CORRUPT` | Corruption after scsReceiveNow | MAJOR if > 0 |
| `m2:GUIDE_CANARY_BEFORE_CORRUPT` | Corruption before guideUpdateNow | MAJOR if > 0 |
| `m2:GUIDE_CANARY_AFTER_CORRUPT` | Corruption after guideUpdateNow | MAJOR if > 0 |

### Interpreting Alarms

**If ISR counters increment:**
- Corruption occurred and was caught by defensive checks
- System did NOT crash (degraded guiding only)
- Note timestamp and active operations

**If canary counters increment:**
- Shows **which direction** corruption came from
- Helps identify if buffer overflow vs targeted write vs DMA

**Example Patterns:**

| Before Canary | Pointer | After Canary | Interpretation |
|--------------|---------|--------------|----------------|
| Corrupted | Valid | OK | Buffer overflow from lower memory |
| OK | Corrupted | OK | Targeted write to pointer only |
| Corrupted | Corrupted | Corrupted | Wide-area corruption (DMA/VME/hardware) |

### When Alarms Fire

1. **Note timestamp** - When did corruption occur?
2. **Check operations** - What was SCS doing? (guiding, chopping, offset, etc.)
3. **Correlate with network** - Any CA commands sent recently?
4. **Check pattern** - Which canaries corrupted? (direction analysis)

---

## Next Steps

### Immediate Actions

1. **Deploy this branch** to production SCS
2. **Monitor EPICS PVs** for corruption detection
3. **Log all alarm events** with timestamps and operations

### When Corruption Detected

1. Note exact time and system state
2. Check which counters incremented (ISR vs canary, which pointer)
3. Review recent operations (guide config, offsets, etc.)
4. Correlate with reflective memory activity
5. Save all diagnostic data for analysis

### Further Investigation (if needed)

- **Hardware watchpoint** - Use JTAG debugger to trap writes to pointer addresses
- **Heap debugging** - Enable RTEMS heap checking (adds overhead)
- **VME bus monitoring** - Check for spurious writes from other nodes
- **Memory dump analysis** - Examine surrounding memory when corruption occurs

---

## References

- [GSFR-43648](https://noirlab.atlassian.net/browse/GSFR-43648) - Master issue: SCS WSOD and focus lost
- [GE7-94](https://noirlab.atlassian.net/browse/GE7-94) - Bug tracking: SCS crash with exception vector 4 or 3
- RTEMS freed memory pattern: 0xa5a5a5a5 (debug fill for freed heap memory)
- Magic numbers: [Wikipedia - Magic number (programming)](https://en.wikipedia.org/wiki/Magic_number_(programming))

---

**Status:** Defensive measures implemented, root cause investigation ongoing
