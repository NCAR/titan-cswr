#pragma once
// this is the format of the memory descriptor list needed by the PLX chip
typedef struct {
	UINT32 dwPADR;	// Local adx.
	UINT32 dwLADR;	// PCI adx.
	UINT32 dwSIZ;	// Bytes to transfer.
	UINT32 dwDPR;	// Next descriptor pointer.
	} DMA_LIST;

typedef struct {
	WD_DMA		*FifoDma;
	WD_DMA		*ListDma;
	DMA_LIST	*pList;
	int			*listaddr;
	} DMA_STUFF; 