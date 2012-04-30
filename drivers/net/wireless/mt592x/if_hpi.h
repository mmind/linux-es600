
#define MCR_HPICSR	0x0018	/* HPI Control Status Register */
#define MCR_eHPICSR	0x0018	/* eHPI Control Status Register */

/* HPI */
#define HPICSR_WAIT_SIG_CTRL_NOT_LOW	(1 << 15)
#define HPICSR_BIG_ENDIAN		(1 << 8)
#define HPICSR_FUNC_PG_MODE		(1 << 2)
#define HPICSR_WD_STS			(1 << 1)

/* eHPI */
#define EHPICSR_FUNC_PG_MODE		(1 << 2)
#define EHPICSR_WD_STS			(1 << 1)
#define EHPICSR_INTBLK			(1 << 0)
