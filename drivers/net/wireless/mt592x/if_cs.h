
/* CF Control Status Register */
#define MCR_CFCSR			0x0018

/* CFCSR */
#define CFCSR_BIG_ENDIAN		(1 << 8)
#define CFCSR_PAGE_SEL			((1 << 6) | (1 << 5) | (1 << 4))
#define CFCSR_FUNC_PG_MODE		(1 << 2)
#define CFCSR_WD_STS			(1 << 1)
