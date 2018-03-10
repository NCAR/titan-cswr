#pragma once
#include "wdc_defs.h"
#include "wdc_lib.h"
#include "hiq_lib.h"
#include "hiq_device.h"

/*************************************************************
  Static functions prototypes
 *************************************************************/
/* -----------------------------------------------
    Device find, open and close
   ----------------------------------------------- */
DWORD LibInit(void);
static DWORD Cleanup(WDC_DEVICE_HANDLE hDev);
WDC_DEVICE_HANDLE DeviceFindAndOpen(DWORD deviceNumber, PHIQ_DEV_CTX pHiqDevCtx);
static BOOL DeviceFind(DWORD deviceNumber, WD_PCI_SLOT *pSlot);
static WDC_DEVICE_HANDLE DeviceOpen(const WD_PCI_SLOT *pSlot, PHIQ_DEV_CTX pHiqDevCtx);
static void DeviceClose(WDC_DEVICE_HANDLE hDev);
//static void printStatus(WDC_DEVICE_HANDLE hDev);
//static void printStatusMsg(WDC_DEVICE_HANDLE hDev, char *aMsg);
//static void printIntStatus(WDC_DEVICE_HANDLE hDev);
//static void printIntStatusMsg(WDC_DEVICE_HANDLE hDev, char *aMsg);

/* -----------------------------------------------
    Interrupt handling
   ----------------------------------------------- */
//static void MenuInterrupts(WDC_DEVICE_HANDLE hDev);
//static void DiagIntHandler(WDC_DEVICE_HANDLE hDev, HIQ_INT_RESULT *pIntResult);

