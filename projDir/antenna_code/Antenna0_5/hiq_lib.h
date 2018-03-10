#ifndef _HIQ_LIB_H_
#define _HIQ_LIB_H_

/************************************************************************
*  File: hiq_lib.h
*
*  Library for accessing HIQ devices using a Kernel PlugIn driver.
*  The code accesses hardware using WinDriver's WDC library.
*  Code was generated by DriverWizard v9.20.
*
*  Jungo Confidential. Copyright (c) 2009 Jungo Ltd.  http://www.jungo.com
*************************************************************************/

#include "wdc_lib.h"
#include "pci_regs.h"
#include "bits.h"

// KPTODO:
// HiQ specific includes.
#include "hiq_hardware.h"
#include "hiq_dma.h"
#include "hiq.h"
//#include "..\include\fifo.h"
//#include "..\hiqdrvr\drvwrapper.h"
#include "common.h"


#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
  General definitions
 *************************************************************/
/* Kernel PlugIn driver name (should be no more than 8 characters) */
#define KP_HIQ_DRIVER_NAME "KP_HIQ"

/* Kernel PlugIn messages - used in WDC_CallKerPlug() calls (user mode) / KP_HIQ_Call() (kernel mode) */
enum {
    KP_HIQ_MSG_VERSION = 1, /* Query the version of the Kernel PlugIn */
    KP_HIQ_MSG_GET_CTX = 2, /* Device context imported into the KP */
};

/* Kernel Plugin messages status */
enum {
    KP_HIQ_STATUS_OK = 0x1,
    KP_HIQ_STATUS_MSG_NO_IMPL = 0x1000,
};

/* Default vendor and device IDs */
#define HIQ_DEFAULT_VENDOR_ID 0x10B5 /* Vendor ID */
#define HIQ_DEFAULT_DEVICE_ID 0x9056 /* Device ID */


/* Kernel PlugIn version information struct */
typedef struct {
    DWORD dwVer;
    CHAR cVer[100];
} KP_HIQ_VERSION;

/* Address space information struct */
#define MAX_TYPE 8
typedef struct {
    DWORD dwAddrSpace;
    CHAR sType[MAX_TYPE];
    CHAR sName[MAX_NAME];
    CHAR sDesc[MAX_DESC];
} HIQ_ADDR_SPACE_INFO;


/* array of HiQ information for handling interrupts from multiple cards */
typedef struct {
//	HIQ		*mH;		// card information for this card.
//	FIFO	*mHIQ;		// fifo information for this card.
	DWORD hCard;		// card Handle.
	BOOL startedDMA;	// TRUE => DMA was Started. Was DMASTARTED 
	BOOL waitingForHF;	// This flag mirrors the waitingForHF enable bit. It is initially enabled. Was HF.
	int pDmaDescriptorIndex;	// index into list of pointers to DMA descriptor structure.
//	int fifoHead;			// FIFO head.
//	int fifoRecordNumber;	// FIFO record number.
    KPTR pDmaDescriptor[FIFORECNUM];	// PLX DMA descriptors.
} HIQ_KP_INT_DATA;

/* Interrupt result information struct */
typedef struct
{
    DWORD dwCounter; /* Number of interrupts received */
    DWORD dwLost;    /* Number of interrupts not yet handled */
    WD_INTERRUPT_WAIT_RESULT waitResult; /* See WD_INTERRUPT_WAIT_RESULT values in windrvr.h */
    DWORD dwEnabledIntType; /* Interrupt type that was actually enabled
                               (MSI/MSI-X/Level Sensitive/Edge-Triggered) */
    DWORD dwLastMessage; /* Message ID of the last received MSI/MSI-X (irrelevant
                            for line-based interrupts) */
/* TODO: You can add fields to HIQ_INT_RESULT to store any additional
         information that you wish to pass to your diagnostics interrupt
         handler routine (DiagIntHandler() in hiq_diag.c) */
	DWORD debugflag;	// temporary
} HIQ_INT_RESULT;

/* HIQ interrupt handler function type */
typedef void (*HIQ_INT_HANDLER)(WDC_DEVICE_HANDLE hDev,
    HIQ_INT_RESULT *pIntResult);

#define INT_MAX_BOARDS	 2	// Maximum number of HiQ receiver cards in the system.

/* HIQ device information struct */
typedef struct {
    HIQ_INT_HANDLER funcIntHandler;
	// Additional data used for interrupt processing in the kernel and user mode.
	HIQ_KP_INT_DATA Hiq_Kp_Int_Data[INT_MAX_BOARDS];
} HIQ_DEV_CTX, *PHIQ_DEV_CTX;

// HIQ register definitions 
enum { HIQ_INTCSR_SPACE = AD_PCI_BAR0 };
enum { HIQ_INTCSR_OFFSET = 0x68 };
enum { HIQ_DMACSR0_SPACE = AD_PCI_BAR0 };
enum { HIQ_DMACSR0_OFFSET = 0xa8 };
enum { HIQ_DMAPR0_SPACE = AD_PCI_BAR0 };
enum { HIQ_DMAPR0_OFFSET = 0x90 };
enum { HIQ_DMAMODE0_SPACE = AD_PCI_BAR0 };
enum { HIQ_DMAMODE0_OFFSET = 0x80 };
enum { HIQ_FIFOCLR_SPACE = AD_PCI_BAR2 };
enum { HIQ_FIFOCLR_OFFSET = 0x1c };
enum { HIQ_ICLEAR_SPACE = AD_PCI_BAR2 };
enum { HIQ_ICLEAR_OFFSET = 0x18 };
enum { HIQ_STATUS_SPACE = AD_PCI_BAR2 };
enum { HIQ_STATUS_OFFSET = 0x10 };
enum { HIQ_TIMER_CMD_012_SPACE = AD_PCI_BAR2 };
enum { HIQ_TIMER_CMD_012_OFFSET = 0x6 };
enum { HIQ_TIMER_CMD_345_SPACE = AD_PCI_BAR2 };
enum { HIQ_TIMER_CMD_345_OFFSET = 0xE };
enum { HIQ_TIMER0_SPACE = AD_PCI_BAR2 };
enum { HIQ_TIMER0_OFFSET = 0x0 };
enum { HIQ_TIMER1_SPACE = AD_PCI_BAR2 };
enum { HIQ_TIMER1_OFFSET = 0x2 };
enum { HIQ_TIMER2_SPACE = AD_PCI_BAR2 };
enum { HIQ_TIMER2_OFFSET = 0x4 };
enum { HIQ_TIMER3_SPACE = AD_PCI_BAR2 };
enum { HIQ_TIMER3_OFFSET = 0x8 };
enum { HIQ_TIMER4_SPACE = AD_PCI_BAR2 };
enum { HIQ_TIMER4_OFFSET = 0xA };
enum { HIQ_TIMER5_SPACE = AD_PCI_BAR2 };
enum { HIQ_TIMER5_OFFSET = 0xC };
enum { HIQ_ISTAT_SPACE = AD_PCI_BAR2 };
enum { HIQ_ISTAT_OFFSET = 0x14 };
enum { HIQ_STATUS1_SPACE = AD_PCI_BAR2 };
enum { HIQ_STATUS1_OFFSET = 0x12 };

/*************************************************************
  Function prototypes
 *************************************************************/
DWORD HIQ_LibInit(void);
DWORD HIQ_LibUninit(void);

#if !defined(__KERNEL__)
WDC_DEVICE_HANDLE HIQ_DeviceOpen(const WD_PCI_CARD_INFO *pDeviceInfo, PHIQ_DEV_CTX pMyDevCtx);
BOOL HIQ_DeviceClose(WDC_DEVICE_HANDLE hDev);

static BOOL IsItemExists(PWDC_DEVICE pDev, ITEM_TYPE item);
DWORD HIQ_IntEnable(WDC_DEVICE_HANDLE hDev, HIQ_INT_HANDLER funcIntHandler);
DWORD HIQ_IntDisable(WDC_DEVICE_HANDLE hDev);
BOOL HIQ_IntIsEnabled(WDC_DEVICE_HANDLE hDev);
#endif

DWORD HIQ_GetNumAddrSpaces(WDC_DEVICE_HANDLE hDev);
BOOL HIQ_GetAddrSpaceInfo(WDC_DEVICE_HANDLE hDev, HIQ_ADDR_SPACE_INFO *pAddrSpaceInfo);

// Function: HIQ_ReadINTCSR()
//   Read from INTCSR register.
UINT32 HIQ_ReadINTCSR (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteINTCSR()
//   Write to INTCSR register.
void HIQ_WriteINTCSR (WDC_DEVICE_HANDLE hDev, UINT32 data);

// Function: HIQ_ReadDMACSR0()
//   Read from DMACSR0 register.
BYTE HIQ_ReadDMACSR0 (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteDMACSR0()
//   Write to DMACSR0 register.
void HIQ_WriteDMACSR0 (WDC_DEVICE_HANDLE hDev, BYTE data);

// Function: HIQ_ReadDMAPR0()
//   Read from DMAPR0 register.
UINT32 HIQ_ReadDMAPR0 (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteDMAPR0()
//   Write to DMAPR0 register.
void HIQ_WriteDMAPR0 (WDC_DEVICE_HANDLE hDev, UINT32 data);

// Function: HIQ_ReadDMAMODE0()
//   Read from DMAMODE0 register.
UINT32 HIQ_ReadDMAMODE0 (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteDMAMODE0()
//   Write to DMAMODE0 register.
void HIQ_WriteDMAMODE0 (WDC_DEVICE_HANDLE hDev, UINT32 data);

// Function: HIQ_ReadFIFOCLR()
//   Read from FIFOCLR register.
BYTE HIQ_ReadFIFOCLR (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteFIFOCLR()
//   Write to FIFOCLR register.
void HIQ_WriteFIFOCLR (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_ReadICLEAR()
//   Read from ICLEAR register.
BYTE HIQ_ReadICLEAR (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteICLEAR()
//   Write to ICLEAR register.
void HIQ_WriteICLEAR (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_ReadSTATUS()
//   Read from STATUS register.
WORD HIQ_ReadSTATUS (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteSTATUS()
//   Write to STATUS register.
void HIQ_WriteSTATUS (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_ReadISTAT()
//   Read from ISTAT register.
BYTE HIQ_ReadISTAT (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteISTAT()
//   Write to ISTAT register.
void HIQ_WriteISTAT (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_ReadSTATUS1()
//   Read from STATUS1 register.
BYTE HIQ_ReadSTATUS1 (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteSTATUS1()
//   Write to STATUS1 register.
void HIQ_WriteSTATUS1 (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_WriteTIMER_CMD_012()
//   Write to TIMER_CMD_012 register.
DWORD HIQ_WriteTIMER_CMD_012 (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_WriteTIMER_CMD_345()
//   Write to TIMER_CMD_345 register.
DWORD HIQ_WriteTIMER_CMD_345 (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_ReadTIMER0()
//   Read from TIMER0 register.
BYTE HIQ_ReadTIMER0 (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteTIMER0()
//   Write to TIMER0 register.
DWORD HIQ_WriteTIMER0 (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_ReadTIMER1()
//   Read from TIMER1 register.
BYTE HIQ_ReadTIMER1 (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteTIMER1()
//   Write to TIMER1 register.
DWORD HIQ_WriteTIMER1 (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_ReadTIMER2()
//   Read from TIMER2 register.
BYTE HIQ_ReadTIMER2 (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteTIMER2()
//   Write to TIMER2 register.
DWORD HIQ_WriteTIMER2 (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_ReadTIMER3()
//   Read from TIMER3 register.
BYTE HIQ_ReadTIMER3 (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteTIMER3()
//   Write to TIMER3 register.
DWORD HIQ_WriteTIMER3 (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_ReadTIMER4()
//   Read from TIMER4 register.
BYTE HIQ_ReadTIMER4 (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteTIMER4()
//   Write to TIMER4 register.
DWORD HIQ_WriteTIMER4 (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_ReadTIMER5()
//   Read from TIMER5 register.
BYTE HIQ_ReadTIMER5 (WDC_DEVICE_HANDLE hDev);

// Function: HIQ_WriteTIMER5()
//   Write to TIMER5 register.
DWORD HIQ_WriteTIMER5 (WDC_DEVICE_HANDLE hDev, WORD data);

// Function: HIQ_timer()
// Program the HIQ timer chips with chip specific parameters
void HIQ_timer(int timernum, int timermode, int count, WDC_DEVICE_HANDLE hDev);

const char *HIQ_GetLastErr(void);

#ifdef __cplusplus
}
#endif

#endif
