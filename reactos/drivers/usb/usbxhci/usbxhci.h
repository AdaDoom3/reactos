#ifndef USBXHCI_H__
#define USBXHCI_H__

#include <ntddk.h>
#include <windef.h>
#include <stdio.h>
#include <wdm.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include "..\usbmport.h"
#include "hardware.h"

extern USBPORT_REGISTRATION_PACKET RegPacket;


//Data structures
typedef struct  _XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY {
   PHYSICAL_ADDRESS ContextBaseAddr [256];
} XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY, *PXHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY;
//----------------------------------------LINK TRB--------------------------------------------------------------------
typedef union _XHCI_LINK_TRB{
    struct {
        ULONG RsvdZ1                     : 4;
        ULONG RingSegmentPointerLo       : 28;
    };
    struct {
        ULONG RingSegmentPointerHi       : 32;
    };
    struct {
        ULONG RsvdZ2                     : 22;
        ULONG InterrupterTarget          : 10;
    };
    struct {
        ULONG CycleBit                  : 1;
        ULONG ToggleCycle               : 1;
        ULONG RsvdZ3                    : 2;
        ULONG ChainBit                  : 1;
        ULONG InterruptOnCompletion     : 1;
        ULONG RsvdZ4                    : 4;
        ULONG TRBType                   : 6;
        ULONG RsvdZ5                    : 16;
    };
    ULONG AsULONG;
} XHCI_LINK_TRB;
//----------------------------------------Command TRBs----------------------------------------------------------------
typedef union _XHCI_COMMAND_NO_OP_TRB {
    struct {
        ULONG RsvdZ1                     : 5;
    };
    struct {
        ULONG RsvdZ2                     : 5;
    };
    struct {
        ULONG RsvdZ3                     : 5;
    };
    struct {
        ULONG CycleBit                  : 1;
        ULONG RsvdZ4                    : 4;
        ULONG TRBType                   : 6;
        ULONG RsvdZ5                    : 14;
    };
    ULONG AsULONG;
} XHCI_COMMAND_NO_OP_TRB;

typedef union _XHCI_COMMAND_TRB {
    XHCI_COMMAND_NO_OP_TRB NoOperation[4];
    XHCI_LINK_TRB Link[4];
}XHCI_COMMAND_TRB, *PXHCI_COMMAND_TRB;

typedef struct _XHCI_COMMAND_RING {
    XHCI_COMMAND_TRB Segment[4];
    PXHCI_COMMAND_TRB CREnquePointer;
    PXHCI_COMMAND_TRB CRDequePointer;
} XHCI_COMMAND_RING;
//----------------------------------------CONTROL TRANSFER DATA STRUCTUERS--------------------------------------------

typedef union _XHCI_CONTROL_SETUP_TRB {
    struct {
        ULONG bmRequestType             : 8;
        ULONG bRequest                  : 8;
        ULONG wValue                    : 16;
    };
    struct {
        ULONG wIndex                    : 16;
        ULONG wLength                   : 16;
    };
    struct {
        ULONG TRBTransferLength         : 17;
        ULONG RsvdZ                     : 5;
        ULONG InterrupterTarget         : 10;
    };
    struct {
        ULONG CycleBit                  : 1;
        ULONG RsvdZ1                    : 4;
        ULONG InterruptOnCompletion     : 1;
        ULONG ImmediateData             : 1;
        ULONG RsvdZ2                    : 3;
        ULONG TRBType                   : 6;
        ULONG TransferType              : 2;
        ULONG RsvdZ3                    : 14;
    };
    ULONG AsULONG;
} XHCI_CONTROL_SETUP_TRB;

typedef union _XHCI_CONTROL_DATA_TRB {
    struct {
        ULONG DataBufferPointerLo       : 32;
    };
    struct {
        ULONG DataBufferPointerHi       : 32;
    };
    struct {
        ULONG TRBTransferLength         : 17;
        ULONG TDSize                    : 5;
        ULONG InterrupterTarget         : 10;
    };
    struct {
        ULONG CycleBit                  : 1;
        ULONG EvaluateNextTRB           : 1;
        ULONG InterruptOnShortPacket    : 1;
        ULONG NoSnoop                   : 1;
        ULONG ChainBit                  : 1;
        ULONG InterruptOnCompletion     : 1;
        ULONG ImmediateData             : 1;
        ULONG RsvdZ1                    : 2;
        ULONG TRBType                   : 6;
        ULONG Direction                 : 1;
        ULONG RsvdZ2                    : 15;
    };
    ULONG AsULONG;
} XHCI_CONTROL_DATA_TRB;

typedef union _XHCI_CONTROL_STATUS_TRB {
    struct {
        ULONG RsvdZ1                    : 32;
    };
    struct {
        ULONG RsvdZ2                    : 32;
    };
    struct {
        ULONG RsvdZ                     : 22;
        ULONG InterrupterTarget         : 10;
    };
    struct {
        ULONG CycleBit                  : 1;
        ULONG EvaluateNextTRB           : 1;
        ULONG ChainBit                  : 2;
        ULONG InterruptOnCompletion     : 1;
        ULONG RsvdZ3                    : 4;
        ULONG TRBType                   : 6;
        ULONG Direction                 : 1;
        ULONG RsvdZ4                    : 15;
    };
    ULONG AsULONG;
} XHCI_CONTROL_STATUS_TRB;

typedef union _XHCI_CONTROL_TRB {
    XHCI_CONTROL_SETUP_TRB  SetupTRB[4];
    XHCI_CONTROL_DATA_TRB   DataTRB[4];
    XHCI_CONTROL_STATUS_TRB StatusTRB[4];
} XHCI_CONTROL_TRB, *PXHCI_CONTROL_TRB;  


//------------------------------------main structs-----------------------
typedef struct _XHCI_EXTENSION {
  ULONG Reserved;
  ULONG Flags;
  PULONG BaseIoAdress;
  PULONG OperationalRegs;
  UCHAR FrameLengthAdjustment;
  BOOLEAN IsStarted;
  USHORT HcSystemErrors;
  ULONG PortRoutingControl;
  USHORT NumberOfPorts; // HCSPARAMS1 => N_PORTS 
  USHORT PortPowerControl; // HCSPARAMS => Port Power Control (PPC)
  
} XHCI_EXTENSION, *PXHCI_EXTENSION;

typedef struct _XHCI_HC_RESOURCES {
  XHCI_DEVICE_CONTEXT_BASE_ADD_ARRAY DCBAA;
  XHCI_COMMAND_RING CommandRing;
} XHCI_HC_RESOURCES, *PXHCI_HC_RESOURCES;

typedef struct _XHCI_ENDPOINT {
  ULONG Reserved;
} XHCI_ENDPOINT, *PXHCI_ENDPOINT;

typedef struct _XHCI_TRANSFER {
  ULONG Reserved;
} XHCI_TRANSFER, *PXHCI_TRANSFER;

//roothub functions
VOID
NTAPI
XHCI_RH_GetRootHubData(
  IN PVOID ohciExtension,
  IN PVOID rootHubData);

MPSTATUS
NTAPI
XHCI_RH_GetStatus(
  IN PVOID ohciExtension,
  IN PUSHORT Status);

MPSTATUS
NTAPI
XHCI_RH_GetPortStatus(
  IN PVOID ohciExtension,
  IN USHORT Port,
  IN PULONG PortStatus);

MPSTATUS
NTAPI
XHCI_RH_GetHubStatus(
  IN PVOID ohciExtension,
  IN PULONG HubStatus);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortReset(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortPower(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortEnable(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_SetFeaturePortSuspend(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnable(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortPower(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspend(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortEnableChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortConnectChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortResetChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortSuspendChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

MPSTATUS
NTAPI
XHCI_RH_ClearFeaturePortOvercurrentChange(
  IN PVOID ohciExtension,
  IN USHORT Port);

VOID
NTAPI
XHCI_RH_DisableIrq(
  IN PVOID ohciExtension);

VOID
NTAPI
XHCI_RH_EnableIrq(
  IN PVOID ohciExtension);


#endif /* USBXHCI_H__ */