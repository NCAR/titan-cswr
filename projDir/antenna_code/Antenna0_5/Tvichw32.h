/* ========================================================================
   =================    TVicHW32  DLL interface        ====================
   =======       Copyright (c) 1997..2004 Victor I.Ishikeev        ========
   ========================================================================
   ==========         mail to tools@entechtaiwan.com            ===========
   ==========         http://www.entechtaiwan.com/tools.htm     ===========
   ======================================================================== */


#ifndef __TVicHW32_H
#define __TVicHW32_H



#ifdef _TVIC_INTERNAL
#define VICFN  __declspec(dllexport) __stdcall
#else
#define VICFN  __stdcall
#endif


#define LPT_NOT_ACQUIRED         0
#define LPT_ACQUIRE_SUCCESS      1
#define LPT_ACQUIRE_REFUSED      2
#define LPT_ACQUIRE_BAD_PORT     3
#define LPT_ACQUIRE_NOT_OPENED   4

#define cmdIoAccess              0
#define cmdMemoryAccess          1
#define cmdBusMaster             2
#define cmdSpecialCycleMonitor   3
#define cmdWriteAndInvalidate    4
#define cmdPaletteSnoop          5
#define cmdParityErrorResponse   6
#define cmdWaitCycle             7
#define cmdSystemError           8
#define cmdFastBackToBack        9


#define PCI_TYPE0_ADDRESSES             6
#define PCI_TYPE1_ADDRESSES             2
#define PCI_TYPE2_ADDRESSES             5

typedef struct _PCI_COMMON_CONFIG {

    USHORT  VendorID;                   // 0 (ro)
    USHORT  DeviceID;                   // 2 (ro)
    USHORT  Command;                    // 4 Device control
    USHORT  Status;                     // 6   
    UCHAR   RevisionID;                 // 8 (ro)
    UCHAR   ProgIf;                     // 9 (ro)
    UCHAR   SubClass;                   //10 (ro)
    UCHAR   BaseClass;                  //11 (ro)
    UCHAR   CacheLineSize;              //12 (ro+)
    UCHAR   LatencyTimer;               //13 (ro+)
    UCHAR   HeaderType;                 //14 (ro)
    UCHAR   BIST;                       //15 Built in self test

    union {

		//
        // Non - Bridge
        //

        struct PCI_HEADER_TYPE_0 {
            ULONG   BaseAddresses[PCI_TYPE0_ADDRESSES];
            ULONG   CardBus_CIS;
            USHORT  subsystem_vendorID;
	    USHORT  subsystem_deviceID;
            ULONG   ROMBaseAddress;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved1[3];
            ULONG   Reserved2;

            UCHAR   InterruptLine;      //
            UCHAR   InterruptPin;       // (ro)
            UCHAR   MinimumGrant;       // (ro)
            UCHAR   MaximumLatency;     // (ro)

        } type0;

        //
        // PCI to PCI Bridge
        //

        struct _PCI_HEADER_TYPE_1 {
            ULONG   BaseAddresses[PCI_TYPE1_ADDRESSES];
            UCHAR   PrimaryBus;
            UCHAR   SecondaryBus;
            UCHAR   SubordinateBus;
            UCHAR   SecondaryLatency;
            UCHAR   IOBase;
            UCHAR   IOLimit;
            USHORT  SecondaryStatus;
            USHORT  MemoryBase;
            USHORT  MemoryLimit;
            USHORT  PrefetchBase;
            USHORT  PrefetchLimit;
            ULONG   PrefetchBaseUpper32;
            ULONG   PrefetchLimitUpper32;
            USHORT  IOBaseUpper16;
            USHORT  IOLimitUpper16;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved1[3];
            ULONG   ROMBaseAddress;
            UCHAR   InterruptLine;
            UCHAR   InterruptPin;
            USHORT  BridgeControl;
        } type1;

        //
        // PCI to CARDBUS Bridge
        //

        struct _PCI_HEADER_TYPE_2 {
            ULONG   SocketRegistersBaseAddress;
            UCHAR   CapabilitiesPtr;
            UCHAR   Reserved;
            USHORT  SecondaryStatus;
            UCHAR   PrimaryBus;
            UCHAR   SecondaryBus;
            UCHAR   SubordinateBus;
            UCHAR   SecondaryLatency;
            struct  {
                ULONG   Base;
                ULONG   Limit;
            }       Range[PCI_TYPE2_ADDRESSES-1];
            UCHAR   InterruptLine;
            UCHAR   InterruptPin;
            USHORT  BridgeControl;
        } type2;


        ULONG AsUlong;

	} u;

    UCHAR   DeviceSpecific[192];

} PCI_COMMON_CONFIG, * PPCI_COMMON_CONFIG;


#ifndef _TVIC_INTERNAL

typedef struct _MSR_DATA 
{
	ULONG MSR_LO;
	ULONG MSR_HI;
} MSR_DATA, * PMSR_DATA;

typedef struct _CPUID_RECORD {
   ULONG EAX;
   ULONG EBX;
   ULONG ECX;
   ULONG EDX;
} CPUID_RECORD; 


typedef struct _IrqClearRec {

	UCHAR ClearIrq;        // 1 - Irq must be cleared, 0 - not
	UCHAR TypeOfRegister;  // 0 - memory-mapped register, 1 - port
	UCHAR WideOfRegister;  // Wide of register : 1 - Byte, 2 - Word, 4 - Double Word
	UCHAR ReadOrWrite;     // 0 - read register to clear Irq, 1 - write
	ULONG RegBaseAddress;  // Memory or port i/o register base address to clear
    ULONG RegOffset;       // Register offset
	ULONG ValueToWrite;    // Value to write (if ReadOrWrite=1)

} IrqClearRec, *pIrqClearRec;

typedef ULONG  (__stdcall * TRing0Function)(PVOID parm);
typedef void   (__stdcall * TKbHookHandler)(BYTE scan_code);
typedef void   (__stdcall * TKbDelphiHandler)(HANDLE Componen, BYTE scan_code);
typedef void   (__stdcall * TOnHWInterrupt)(USHORT IRQNumber);
typedef void   (__stdcall * TOnDelphiInterrupt)(HANDLE Component, WORD IRQNumber);

typedef void   (__stdcall * TOnHWInterruptEx)(USHORT IRQNumber,pIrqClearRec IrqRec);
typedef void   (__stdcall * TOnDelphiInterruptEx)(HANDLE Component, WORD IRQNumber,pIrqClearRec IrqRec);

typedef BOOL   (__stdcall * TEnumerateCallBack)(HANDLE HW32,
												BYTE bus,
												BYTE dev,
												BYTE func,
												PPCI_COMMON_CONFIG pCfg);


//===== DMA buffer request ==

typedef struct _DMA_BUFFER_REQUEST {
	ULONG LengthOfBuffer;
	ULONG AlignMask;
	ULONG PhysDmaAddress;
	ULONG LinDmaAddress;
	ULONG PMemHandle;
	ULONG Rezerved1;
	ULONG KernelDmaAddress;
	ULONG Rezerved2;
} DMA_BUFFER_REQUEST, * PDMA_BUFFER_REQUEST;


typedef struct _FIFO_RECORD {
    ULONG  PortAddr;
    ULONG  NumPorts;
    UCHAR Buf[1];
} FIFO_RECORD, *PFIFO_RECORD;

typedef struct _FIFO_RECORDW {
    ULONG  PortAddr;
    ULONG   NumPorts;
    USHORT Buf[1];
} FIFO_RECORDW, *PFIFO_RECORDW;

typedef struct _FIFO_RECORDL {
    ULONG  PortAddr;
    ULONG    NumPorts;
    ULONG   Buf[1];
} FIFO_RECORDL, *PFIFO_RECORDL;


#define FIFOSIZE 1000

typedef struct _KEYBOARD_INPUT_DATA 
{
    USHORT UnitId;
    USHORT MakeCode;
    USHORT Flags;
    USHORT Reserved;
    ULONG ExtraInformation;
} KEYBOARD_INPUT_DATA, * PKEYBOARD_INPUT_DATA;



#endif  // _TVIC_INTERNAL

typedef struct _HDDInfo {

       DWORD      BufferSize;
       DWORD      DoubleTransfer;
       DWORD      ControllerType;
       DWORD      ECCMode;
       DWORD      SectorsPerInterrupt;
       DWORD      Cylinders;
       DWORD      Heads;
       DWORD      SectorsPerTrack;
       char       Model[41];
       char       SerialNumber[21];
       char       Revision[9];

} HDDInfo, *pHDDInfo;

#ifndef _TVIC_INTERNAL
extern "C" {
#endif

ULONG   VICFN GetLocalInstance();

HANDLE  VICFN OpenTVicHW();
HANDLE  VICFN OpenTVicHW32(HANDLE HW32, 
						   char * ServiceName,
						   char * EntryPoint);
HANDLE  VICFN CloseTVicHW32(HANDLE HW32);

BOOL    VICFN GetActiveHW(HANDLE HW32); 
		
BOOL    VICFN GetHardAccess(HANDLE);
void    VICFN SetHardAccess(HANDLE HW32, BOOL bNewValue);

UCHAR   VICFN GetPortByte(HANDLE HW32,ULONG PortAddr); 
void    VICFN SetPortByte(HANDLE HW32,ULONG PortAddr, UCHAR nNewValue);
USHORT  VICFN GetPortWord(HANDLE HW32,ULONG PortAddr);
void    VICFN SetPortWord(HANDLE HW32,ULONG PortAddr, USHORT nNewValue);
ULONG   VICFN GetPortLong(HANDLE HW32,ULONG PortAddr);
void    VICFN SetPortLong(HANDLE HW32,ULONG PortAddr, ULONG  nNewValue);

ULONG   VICFN MapPhysToLinear(HANDLE HW32,ULONG PhAddr, ULONG PhSize); 
void    VICFN UnmapMemory(HANDLE HW32,ULONG PhAddr, ULONG PhSize);

UCHAR   VICFN GetMem(HANDLE HW32, ULONG MappedAddr, ULONG Offset);
void    VICFN SetMem(HANDLE HW32, ULONG MappedAddr, ULONG Offset, UCHAR nNewValue);
USHORT  VICFN GetMemW(HANDLE HW32,ULONG MappedAddr, ULONG Offset);
void    VICFN SetMemW(HANDLE HW32,ULONG MappedAddr, ULONG Offset, USHORT nNewValue);
ULONG   VICFN GetMemL(HANDLE HW32,ULONG MappedAddr, ULONG Offset);
void    VICFN SetMemL(HANDLE HW32,ULONG MappedAddr, ULONG Offset, ULONG nNewValue);

PVOID   VICFN GetLockedMemory(HANDLE HW32);
ULONG   VICFN GetTempVar(HANDLE HW32);

BOOL    VICFN IsIRQMasked(HANDLE HW32, USHORT IRQNumber); 

void    VICFN UnmaskIRQ(HANDLE HW32, USHORT IRQNumber, TOnHWInterrupt HWHandler); 
void    VICFN UnmaskIRQEx(HANDLE HW32, 
						  USHORT IRQNumber,
						  ULONG IrqShared,
						  TOnHWInterrupt HWHandler,
						  pIrqClearRec   IrqRec); 

void    VICFN UnmaskDelphiIRQ(HANDLE HW32, HANDLE Component, USHORT IRQNumber, TOnHWInterrupt HWHandler); 
void    VICFN UnmaskDelphiIRQEx(HANDLE HW32, 
								HANDLE Component, 
								USHORT IRQNumber, 
								ULONG  IrqShared,
								TOnHWInterrupt HWHandler,
								pIrqClearRec IrqRec); 

ULONG   VICFN MaskIRQ(HANDLE HW32, USHORT IRQNumber); 
ULONG   VICFN GetIRQCounter(HANDLE HW32, USHORT IRQNumber);
void	VICFN PulseIrqKernelEvent(HANDLE HW32);
void	VICFN PulseIrqLocalEvent(HANDLE HW32);

void    VICFN HookKeyboard(HANDLE HW32, TKbHookHandler HWHandler);
void    VICFN HookDelphiKeyboard(HANDLE HW32, HANDLE Component, TKbHookHandler HWHandler);
void    VICFN UnhookKeyboard(HANDLE HW32);
void    VICFN PutScanCode(HANDLE HW32,UCHAR b);
WORD    VICFN GetScanCode(HANDLE HW32);
void    VICFN PulseKeyboard(HANDLE HW32);
void    VICFN PulseKeyboardLocal(HANDLE HW32);

UCHAR   VICFN GetLPTNumber(HANDLE HW32);
void    VICFN SetLPTNumber(HANDLE HW32,UCHAR nNewValue);
UCHAR   VICFN GetLPTNumPorts(HANDLE HW32);
ULONG   VICFN GetLPTBasePort(HANDLE HW32);
UCHAR   VICFN AddNewLPT(HANDLE HW32, USHORT PortBaseAddress);


BOOL    VICFN GetPin(HANDLE HW32,UCHAR nPin);
void    VICFN SetPin(HANDLE HW32,UCHAR nPin, BOOL bNewValue);

BOOL    VICFN GetLPTAckwl(HANDLE HW32);
BOOL    VICFN GetLPTBusy(HANDLE HW32);
BOOL    VICFN GetLPTPaperEnd(HANDLE HW32);
BOOL    VICFN GetLPTSlct(HANDLE HW32);
BOOL    VICFN GetLPTError(HANDLE HW32); 

void    VICFN LPTInit(HANDLE HW32);
void    VICFN LPTSlctIn(HANDLE HW32);
void    VICFN LPTStrobe(HANDLE HW32);
void    VICFN LPTAutofd(HANDLE HW32,BOOL Flag);

BOOL    VICFN LPTPrintChar(HANDLE HW32, UCHAR ch);

void    VICFN ForceIrqLPT(HANDLE HW32,BOOL IrqEnable);

void    VICFN SetLPTReadMode(HANDLE HW32);
void    VICFN SetLPTWriteMode(HANDLE HW32);


ULONG   VICFN DebugCode(HANDLE HW32);

void	VICFN ReadPortFIFO ( HANDLE         HW32,
						   PFIFO_RECORD   pPortRec);
                           
void	VICFN ReadPortWFIFO( HANDLE         HW32,
						   PFIFO_RECORDW  pPortRec);
                           
void	VICFN ReadPortLFIFO( HANDLE         HW32,
						   PFIFO_RECORDL  pPortRec);
                           
void	VICFN WritePortFIFO ( HANDLE        HW32,
						    PFIFO_RECORD  pPortRec);
                           
void	VICFN WritePortWFIFO( HANDLE        HW32,
							PFIFO_RECORDW pPortRec);
                           
void	VICFN WritePortLFIFO( HANDLE        HW32,
						    PFIFO_RECORDL pPortRec);
                           

void	VICFN GetHDDInfo(HANDLE   HW32,
					   USHORT   IdeNumber, 
					   USHORT   Master,
					   pHDDInfo Info);


USHORT VICFN GetLastPciBus(HANDLE HW32);
USHORT VICFN GetHardwareMechanism(HANDLE HW32);

BOOL   VICFN GetPciDeviceInfo( HANDLE              HW32,
	    					   USHORT              BusNum,
							   USHORT              DeviceNum,
                               USHORT              FuncNum,
                               PPCI_COMMON_CONFIG  pPciCfg);


BOOL   VICFN GetPciHeader( HANDLE              HW32,
                           ULONG               VendorId,
				           ULONG               DeviceId,
				           ULONG               OffsetInBytes,
				           ULONG               LengthInBytes,
	    			       PPCI_COMMON_CONFIG  pPciCfg);

BOOL   VICFN SetPciHeader( HANDLE              HW32,
                           ULONG               VendorId,
				           ULONG               DeviceId,
				           ULONG               OffsetInBytes,
				           ULONG               LengthInBytes,
	    			       PPCI_COMMON_CONFIG  pPciCfg);

void   VICFN EnumeratePciDevices(HANDLE             HW32,
					   TEnumerateCallBack cbFunc);


BOOL   VICFN GetSysDmaBuffer( HANDLE                HW32,
	 					      DMA_BUFFER_REQUEST  * BufReq);

BOOL   VICFN GetBusmasterDmaBuffer( HANDLE              HW32,
	 					            PDMA_BUFFER_REQUEST BufReq);

void   VICFN FreeDmaBuffer( HANDLE              HW32,
	 			      	    PDMA_BUFFER_REQUEST BufReq);

USHORT VICFN AcquireLPT    ( HANDLE HW32, USHORT LPTNumber);

void   VICFN ReleaseLPT    ( HANDLE HW32, USHORT LPTNumber);			      

USHORT VICFN IsLPTAcquired ( HANDLE HW32, USHORT LPTNumber);			      

ULONG  VICFN RunRing0Function(HANDLE HW32,
							  TRing0Function     R0func,
							  PVOID              pParm);

void  VICFN GetMsrValue(HANDLE HW32, ULONG RegNumber, PMSR_DATA MsrData); 
void  VICFN CPUID(HANDLE HW32, CPUID_RECORD * Rec); 


#ifndef _TVIC_INTERNAL
} // extern "C"
#endif

#endif
