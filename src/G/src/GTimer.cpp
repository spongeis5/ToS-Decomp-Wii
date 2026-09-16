// G/src/GTimer.cpp -- the third bootstrap unit.
//
// Two functions, read off tools/disasm.py --unit G/src/GTimer.cpp:
//
//   GetTicks__6GTimerFv          4 B    b GetProfileTicks__6GTimerFv
//   GetProfileTicks__6GTimerFv  84 B
//
// GetTicks is a four-byte tail branch, which is a non-inline member whose
// whole body is a call to the other with the same (empty) arguments -- the
// same shape as GHeapStarter's two forwarders.
//
// GetProfileTicks is the Revolution SDK's ticks-to-microseconds conversion
// over the bus clock. The listing:
//
//     bl     OSGetTime           64-bit result in r3:r4
//     lis    r6,0x8000
//     lwz    r0,248(r6)          = 0x800000F8, the bus clock
//     srwi   r0,r0,2             / 4  -- the timer clock
//     lis    r5,0x431C
//     addi   r5,r5,-8573         = 0x431BDE83
//     mulhwu r0,r5,r0
//     srwi   r6,r0,15            magic divide: 2^47 / 0x431BDE83 = 125000
//     slwi   r3,r3,3             the 64-bit numerator is ticks << 3,
//     slwi   r4,r4,3             i.e. ticks * 8, with
//     rlwimi r3,r6,3,29,31       the top three bits of the low word
//     li     r5,0                carried into the high word
//     bl     __div2i             (r3:r4) / (r5:r6)
//
// so the source is `OSGetTime() * 8 / (OS_TIMER_CLOCK / 125000)`, which is
// OSTicksToMicroseconds spelled out, with OS_TIMER_CLOCK = OS_BUS_CLOCK / 4
// and OS_BUS_CLOCK the word at 0x800000F8. The divide is a call because the
// denominator is not a compile-time constant.

typedef long long OSTime;

extern "C" OSTime OSGetTime(void);

#define OS_BUS_CLOCK (*(volatile unsigned long*)0x800000F8)
#define OS_TIMER_CLOCK (OS_BUS_CLOCK / 4)

class GTimer {
public:
    static OSTime GetTicks();
    static OSTime GetProfileTicks();
};

OSTime GTimer::GetTicks()
{
    return GetProfileTicks();
}

OSTime GTimer::GetProfileTicks()
{
    return OSGetTime() * 8 / (OS_TIMER_CLOCK / 125000);
}
