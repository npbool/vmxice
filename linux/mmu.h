#include "define.h"

#ifndef MMU_H
#define MMU_H

typedef struct {
	unsigned	Present:1;
	unsigned	Writable:1;
	unsigned	Owner:1;
	unsigned	WriteThrough:1;
	unsigned	CacheDisable:1;
	unsigned	Accessed:1;
	unsigned	Dirty:1;
	unsigned	LargePage:1;
	unsigned	Global:1;
	unsigned	Ignored9_11:3;
	unsigned	PAT:1;
	unsigned	Reserved13_21:9;
	unsigned	PhysFrame:10;
}PDE4M,*PPDE4M;

typedef struct {
	unsigned	Present:1;
	unsigned	Writable:1;
	unsigned	Owner:1;
	unsigned	WriteThrough:1;
	unsigned	CacheDisable:1;
	unsigned	Accessed:1;
	unsigned	Dirty:1;
	unsigned	LargePage:1;
	unsigned	Ignored8_11:4;
	unsigned	PageTable:20;
}PDE,*PPDE;

typedef struct {
	unsigned	Present:1;
	unsigned	Writable:1;
	unsigned	Owner:1;
	unsigned	WriteThrough:1;
	unsigned	CacheDisable:1;
	unsigned	Accessed:1;
	unsigned	Dirty:1;
	unsigned	Reserved:1;
	unsigned	Global:1;
	unsigned	Ignored9_11:3;
	unsigned	PhysFrame:20;
}PTE,*PPTE;


ULONG GetPhysAddress(ULONG CR3,ULONG VirtAddress,PULONG PhysAddress);
VOID GetVirtAddress(ULONG CR3,ULONG PhysAddress);
ULONG GetPde(ULONG CR3,ULONG VirtAddress);
ULONG GetPte(ULONG CR3,ULONG VirtAddress);
ULONG MapVmxiceToGuest(ULONG DstCR3,ULONG SrcCR3);
ULONG UnmapVmxiceFromGuest(ULONG DstCR3);
ULONG IsAddressExist(ULONG VirtAddress);
ULONG IsAddressRangeExist(ULONG VirtAddress,ULONG Len);

#endif
