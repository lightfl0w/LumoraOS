#ifndef BOOT_EFI_H
#define BOOT_EFI_H

#include <stdint.h>

typedef void *EFI_HANDLE;
typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint16_t CHAR16;

typedef int64_t EFI_STATUS;
#define EFI_SUCCESS 0

typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} EFI_GUID;

typedef struct {
    uint64_t Signature;
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t CRC32;
    uint32_t Reserved;
} EFI_TABLE_HEADER;

typedef struct {
    EFI_TABLE_HEADER Hdr;

    void *Reset;
    int64_t (*OutputString)(void *, CHAR16 *);
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef struct {
    uint32_t Type;
    uint32_t Pad;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_PHYSICAL_ADDRESS VirtualStart;
    uint64_t Pages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    void *RaiseTPL;
    void *RestoreTPL;
    int64_t (*AllocatePages)(int, int, uint64_t, EFI_PHYSICAL_ADDRESS *);
    void *FreePages;
    int64_t (*GetMemoryMap)(uint64_t *, EFI_MEMORY_DESCRIPTOR *, uint64_t *,
                            uint64_t *, uint32_t *);
    int64_t (*AllocatePool)(int, uint64_t, void **);
    void (*FreePool)(void *);
    void *CreateEvent;
    void *SetTimer;
    void *WaitForEvent;
    void *SignalEvent;
    void *CloseEvent;
    void *CheckEvent;
    void *InstallProtocolInterface;
    void *ReinstallProtocolInterface;
    void *UninstallProtocolInterface;
    int64_t (*HandleProtocol)(EFI_HANDLE, const EFI_GUID *, void **);
    void *Reserved;
    void *RegisterProtocolNotify;
    void *LocateHandle;
    void *LocateDevicePath;
    void *InstallConfigurationTable;
    void *LoadImage;
    void *StartImage;
    void *Exit;
    void *UnloadImage;
    int64_t (*ExitBootServices)(EFI_HANDLE, uint64_t);
    void *GetNextMonotonicCount;
    int64_t (*Stall)(uint64_t);
    int64_t (*SetWatchdogTimer)(uint64_t, uint64_t, uint64_t, CHAR16 *);
    void *ConnectController;
    void *DisconnectController;
    void *OpenProtocol;
    void *CloseProtocol;
    void *OpenProtocolInformation;
    void *ProtocolsPerHandle;
    int64_t (*LocateHandleBuffer)(int, const EFI_GUID *, void *, uint64_t *,
                                  EFI_HANDLE **);
    void *LocateProtocol;
    void *InstallMultipleProtocolInterfaces;
    void *UninstallMultipleProtocolInterfaces;
    void *CalculateCrc32;
    void (*CopyMem)(void *, const void *, uint64_t);
    void (*SetMem)(void *, uint64_t, uint8_t);
    void *CreateEventEx;
} EFI_BOOT_SERVICES;

#define EFI_PAGE_SIZE 4096u
#define EFI_SIZE_TO_PAGES(n) (((n) + EFI_PAGE_SIZE - 1) / EFI_PAGE_SIZE)

/* EfiAllocateType */
#define EFI_ALLOCATE_ANY_PAGES 0
#define EFI_ALLOCATE_ADDRESS 2

/* EfiMemoryType */
#define EFI_LOADER_DATA 2
#define EFI_BOOT_SERVICES_DATA 4
#define EFI_CONVENTIONAL 7
#define EFI_ACPI_RECLAIM 9
#define EFI_ACPI_NVS 10

typedef struct {
    uint64_t Revision;
    EFI_HANDLE ParentHandle;
    EFI_HANDLE SystemTable;
    EFI_HANDLE DeviceHandle;
    void *FilePath;
    void *Reserved;
    uint32_t LoadOptionsSize;
    void *LoadOptions;
    void *ImageBase;
    uint64_t ImageSize;
    uint32_t ImageCodeType;
    uint32_t ImageDataType;
    void *Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct {
    uint64_t Revision;
    int64_t (*Open)(void *, void **, CHAR16 *, uint64_t, uint64_t);
    int64_t (*Close)(void *);
    void *Delete;
    int64_t (*Read)(void *, uint64_t *, void *);
    void *Write;
    int64_t (*GetPosition)(void *, uint64_t *);
    int64_t (*SetPosition)(void *, uint64_t);
    int64_t (*GetInfo)(void *, const EFI_GUID *, uint64_t *, void *);
    void *SetInfo;
    void *Flush;
} EFI_FILE_PROTOCOL;

typedef struct {
    uint64_t Revision;
    int64_t (*OpenVolume)(void *, EFI_FILE_PROTOCOL **);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

#define EFI_FILE_MODE_READ 0x0000000000000001ull

typedef struct {
    uint32_t Version;
    uint32_t HorizontalResolution;
    uint32_t VerticalResolution;
    uint32_t PixelFormat;
    struct {
        uint32_t RedMask;
        uint32_t GreenMask;
        uint32_t BlueMask;
        uint32_t ReservedMask;
    } PixelInformation;
    uint32_t PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    uint64_t MaxMode;
    uint64_t Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    uint64_t SizeOfInfo;
    EFI_PHYSICAL_ADDRESS FrameBufferBase;
    uint64_t FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct {
    void *QueryMode;
    void *SetMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    uint32_t FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    void *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    void *StdErr;
    void *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    uint64_t NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

#define EFI_LOCATE_BY_PROTOCOL 2

static const EFI_GUID EFI_LOADED_IMAGE_PROTOCOL_GUID = {
    0x5b1b31a1,
    0x9562,
    0x11d2,
    {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

static const EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID = {
    0x964e5b22,
    0x6459,
    0x11d2,
    {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

static const EFI_GUID EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID = {
    0x9042a9de,
    0x23dc,
    0x4a38,
    {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}};

#endif
