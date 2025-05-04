//
// Stubbish cputest replacement for supafaust
//
enum
{
 CPUTEST_FLAG_AVX = 1U << 0,
 CPUTEST_FLAG_SSE = 1U << 1
};

static INLINE uint32 cputest_get_flags(void)
{
 uint32 ret = 0;

 #ifdef ARCH_X86
 ret |= CPUTEST_FLAG_SSE;
 #endif

 return ret;
}
