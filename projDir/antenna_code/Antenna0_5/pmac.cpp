#include "pci_w32.h"

static unsigned short *gPMACDPRAM = NULL;
static PCI_CARD *gPMAC = NULL;

/**
 * Initialize the PMAC PCI interface and return the pointer to the DPRAM
 */
unsigned short *get_pmac_dpram()
{
	if( !gPMACDPRAM )
	{
		init_pci();
		gPMAC = find_pci_card(PMAC_VENDORID,PMAC_DEVICEID,0);
		gPMACDPRAM = (unsigned short *)(pci_card_membase(gPMAC,2,0x4000)) + 0x400;
	}
	return(gPMACDPRAM);
}

void close_pmac()
{
	if( gPMAC )
	{
		delete_pci_card(gPMAC);
	}
}