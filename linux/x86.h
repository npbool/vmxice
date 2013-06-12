/*
 * Intel CPU features in CR4
 */

/*
#define X86_CR4_VME			0x0001  // enable vm86 extensions
#define X86_CR4_PVI			0x0002  // virtual interrupts flag enable
#define X86_CR4_TSD			0x0004  // disable time stamp at ipl 3
#define X86_CR4_DE			0x0008  // enable debugging extensions
#define X86_CR4_PSE			0x0010  // enable page size extensions
#define X86_CR4_PAE			0x0020  // enable physical address extensions
#define X86_CR4_MCE			0x0040  // Machine check enable
#define X86_CR4_PGE			0x0080  // enable global pages
#define X86_CR4_PCE			0x0100  // enable performance counters at ipl 3
#define X86_CR4_OSFXSR		0x0200  // enable fast FPU save and restore
#define X86_CR4_OSXMMEXCPT	0x0400  // enable unmasked SSE exceptions
#define X86_CR4_VMXE		0x2000  // enable VMX
*/

/*
 * Intel CPU  MSR
 * MSRs & bits used for VMX enabling
 */

/*
#define MSR_IA32_VMX_BASIC   			0x480
#define MSR_IA32_FEATURE_CONTROL 		0x03a
#define MSR_IA32_VMX_PINBASED_CTLS		0x481
#define MSR_IA32_VMX_PROCBASED_CTLS		0x482
#define MSR_IA32_VMX_EXIT_CTLS			0x483
#define MSR_IA32_VMX_ENTRY_CTLS			0x484
#define MSR_IA32_SYSENTER_CS			0x174
#define MSR_IA32_SYSENTER_ESP			0x175
#define MSR_IA32_SYSENTER_EIP			0x176
*/
#define MSR_IA32_DEBUGCTL				0x1d9

#define FLAGS_CF_MASK (1 << 0)
#define FLAGS_PF_MASK (1 << 2)
#define FLAGS_AF_MASK (1 << 4)
#define FLAGS_ZF_MASK (1 << 6)
#define FLAGS_SF_MASK (1 << 7)
#define FLAGS_TF_MASK (1 << 8)
#define FLAGS_IF_MASK (1 << 9)
#define FLAGS_RF_MASK (1 << 16)

#define IDT_TYPE_TASK_GATE    0b00101
#define IDT_TYPE_32_INT_GATE  0b01110
#define IDT_TYPE_16_INT_GATE  0b00110
#define IDT_TYPE_32_TRAP_GATE 0b01111
#define IDT_TYPE_16_TRAP_GATE 0b00111

typedef struct {
	unsigned      LowOffset       :16;
	unsigned      Selector        :16;
//	union{
//		unsigned      Access      :16;
//		struct{
			unsigned      High_0_8        :8;
			unsigned      GateType        :3;
			unsigned      GateSize        :1;
			unsigned      Bit12           :1;
			unsigned      DPL             :2;
			unsigned      P               :1;
//		};
//	};
	unsigned      HighOffset      :16;
} IDT_ENTRY, *PIDT_ENTRY;

typedef struct _IDTR {
	unsigned	Limit		:16;
	unsigned	BaseLo		:16;
	unsigned	BaseHi		:16;
} IDTR;

typedef struct {
	unsigned	CF		:1;
	unsigned	Bit1	:1;
	unsigned	PF		:1;
	unsigned	Bit3	:1;
	unsigned	AF		:1;
	unsigned	Bit5	:1;
	unsigned	ZF		:1;
	unsigned	SF		:1;
	unsigned	TF		:1;
	unsigned	IF		:1;
	unsigned	DF		:1;
	unsigned	OF		:1;
	unsigned	IOPL	:2;
	unsigned	NT		:1;
	unsigned	Bit15	:1;
	unsigned	RF		:1;
	unsigned	VM		:1;
	unsigned	AC		:1;
	unsigned	VIF		:1;
	unsigned	VIP		:1;
	unsigned	ID		:1;
	unsigned	Reserved	:10;
} EFLAGS, *PEFLAGS;

typedef struct {
	unsigned	L0:1;
	unsigned	G0:1;
	unsigned	L1:1;
	unsigned	G1:1;
	unsigned	L2:1;
	unsigned	G2:1;
	unsigned	L3:1;
	unsigned	G3:1;
	unsigned	LE:1;
	unsigned	GE:1;
	unsigned	Bit10:1;
	unsigned	Bit11:1;
	unsigned	Bit12:1;
	unsigned	GD:1;
	unsigned	Bit14:1;
	unsigned	Bit15:1;
	unsigned	RWE0:2;
	unsigned	LEN0:2;
	unsigned	RWE1:2;
	unsigned	LEN1:2;
	unsigned	RWE2:2;
	unsigned	LEN2:2;
	unsigned	RWE3:2;
	unsigned	LEN3:2;
} DR7, *PDR7;

typedef struct {
	unsigned	B0:1;
	unsigned	B1:1;
	unsigned	B2:1;
	unsigned	B3:1;
	unsigned	Bit4_12:10;
	unsigned	BD:1;
	unsigned	BS:1;
	unsigned	BT:1;
	unsigned	Bit16_31:16;
} DR6, *PDR6;

typedef struct {
	unsigned	PE:1;
	unsigned	MP:1;
	unsigned	EM:1;
	unsigned	TS:1;
	unsigned	ET:1;
	unsigned	NE:1;
	unsigned	Bit6_15:10;
	unsigned	WP:1;
	unsigned	Bit17:1;
	unsigned	AM:1;
	unsigned	Bit19_28:12;
	unsigned	NW:1;
	unsigned	CD:1;
	unsigned	PG:1;
} CR0, *PCR0;

typedef struct {
	unsigned	VME:1;
	unsigned	PVI:1;
	unsigned	TSD:1;
	unsigned	DE:1;
	unsigned	PSE:1;
	unsigned	PAE:1;
	unsigned	MCE:1;
	unsigned	PGE:1;
	unsigned	PCE:1;
	unsigned	OSFXSR:1;
	unsigned	OSXMMEXCPT:1;
	unsigned	Bit11:1;
	unsigned	Bit12:1;
	unsigned	VMXE:1;
	unsigned	SMXE:1;
	unsigned	Bit15:1;
	unsigned	Bit16:1;
	unsigned	PCIDE:1;
	unsigned	OSXSAVE:1;
	unsigned	Bit19:1;
	unsigned	SMEP:1;
	unsigned	Bit20_31:11;
} CR4, *PCR4;
