/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/disk.c
 * PURPOSE:         Disk and Drive functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Erik Bos, Alexandre Julliard :
 *                      GetLogicalDriveStringsA,
 *                      GetLogicalDriveStringsW, GetLogicalDrives
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
//WINE copyright notice:
/*
 * DOS drives handling functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
 */

#include <k32.h>
#include <strsafe.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(kernel32file);

#define MAX_DOS_DRIVES 26

/*
 * @implemented
 */
/* Synced to Wine-2008/12/28 */
DWORD
WINAPI
GetLogicalDriveStringsA(IN DWORD nBufferLength,
                        IN LPSTR lpBuffer)
{
    DWORD drive, count;
    DWORD dwDriveMap;
    LPSTR p;

    dwDriveMap = GetLogicalDrives();

    for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
    {
        if (dwDriveMap & (1<<drive))
            count++;
    }


    if ((count * 4) + 1 > nBufferLength) return ((count * 4) + 1);

    p = lpBuffer;

    for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
        if (dwDriveMap & (1<<drive))
        {
            *p++ = 'A' + (UCHAR)drive;
            *p++ = ':';
            *p++ = '\\';
            *p++ = '\0';
        }
    *p = '\0';

    return (count * 4);
}

/*
 * @implemented
 */
/* Synced to Wine-2008/12/28 */
DWORD
WINAPI
GetLogicalDriveStringsW(IN DWORD nBufferLength,
                        IN LPWSTR lpBuffer)
{
    DWORD drive, count;
    DWORD dwDriveMap;
    LPWSTR p;

    dwDriveMap = GetLogicalDrives();

    for (drive = count = 0; drive < MAX_DOS_DRIVES; drive++)
    {
        if (dwDriveMap & (1<<drive))
            count++;
    }

    if ((count * 4) + 1 > nBufferLength) return ((count * 4) + 1);

    p = lpBuffer;
    for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
        if (dwDriveMap & (1<<drive))
        {
            *p++ = (WCHAR)('A' + drive);
            *p++ = (WCHAR)':';
            *p++ = (WCHAR)'\\';
            *p++ = (WCHAR)'\0';
        }
    *p = (WCHAR)'\0';

    return (count * 4);
}

/*
 * @implemented
 */
/* Synced to Wine-? */
DWORD
WINAPI
GetLogicalDrives(VOID)
{
    NTSTATUS Status;
    PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;

    /* Get the Device Map for this Process */
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDeviceMap,
                                       &ProcessDeviceMapInfo,
                                       sizeof(ProcessDeviceMapInfo),
                                       NULL);

    /* Return the Drive Map */
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return ProcessDeviceMapInfo.Query.DriveMap;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDiskFreeSpaceA(IN LPCSTR lpRootPathName,
                  OUT LPDWORD lpSectorsPerCluster,
                  OUT LPDWORD lpBytesPerSector,
                  OUT LPDWORD lpNumberOfFreeClusters,
                  OUT LPDWORD lpTotalNumberOfClusters)
{
    PCSTR RootPath;
    PUNICODE_STRING RootPathU;

    RootPath = lpRootPathName;
    if (RootPath == NULL)
    {
        RootPath = "\\";
    }

    RootPathU = Basep8BitStringToStaticUnicodeString(RootPath);
    if (RootPathU == NULL)
    {
        return FALSE;
    }

    return GetDiskFreeSpaceW(RootPathU->Buffer, lpSectorsPerCluster,
                             lpBytesPerSector, lpNumberOfFreeClusters,
                             lpTotalNumberOfClusters);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDiskFreeSpaceW(IN LPCWSTR lpRootPathName,
                  OUT LPDWORD lpSectorsPerCluster,
                  OUT LPDWORD lpBytesPerSector,
                  OUT LPDWORD lpNumberOfFreeClusters,
                  OUT LPDWORD lpTotalNumberOfClusters)
{
    BOOL Below2GB;
    PCWSTR RootPath;
    NTSTATUS Status;
    HANDLE RootHandle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_FS_SIZE_INFORMATION FileFsSize;

    /* If no path provided, get root path */
    RootPath = lpRootPathName;
    if (lpRootPathName == NULL)
    {
        RootPath = L"\\";
    }

    /* Convert the path to NT path */
    if (!RtlDosPathNameToNtPathName_U(RootPath, &FileName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Open it for disk space query! */
    InitializeObjectAttributes(&ObjectAttributes, &FileName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&RootHandle, SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_FREE_SPACE_QUERY);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        RtlFreeHeap(RtlGetProcessHeap(), 0, FileName.Buffer);
        if (lpBytesPerSector != NULL)
        {
            *lpBytesPerSector = 0;
        }

        return FALSE;
    }

    /* We don't need the name any longer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, FileName.Buffer);

    /* Query disk space! */
    Status = NtQueryVolumeInformationFile(RootHandle, &IoStatusBlock, &FileFsSize,
                                          sizeof(FILE_FS_SIZE_INFORMATION),
                                          FileFsSizeInformation);
    NtClose(RootHandle);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Are we in some compatibility mode where size must be below 2GB? */
    Below2GB = ((NtCurrentPeb()->AppCompatFlags.LowPart & GetDiskFreeSpace2GB) == GetDiskFreeSpace2GB);

    /* If we're to overflow output, make sure we return the maximum */
    if (FileFsSize.TotalAllocationUnits.HighPart != 0)
    {
        FileFsSize.TotalAllocationUnits.LowPart = -1;
    }

    if (FileFsSize.AvailableAllocationUnits.HighPart != 0)
    {
        FileFsSize.AvailableAllocationUnits.LowPart = -1;
    }

    /* Return what user asked for */
    if (lpSectorsPerCluster != NULL)
    {
        *lpSectorsPerCluster = FileFsSize.SectorsPerAllocationUnit;
    }

    if (lpBytesPerSector != NULL)
    {
        *lpBytesPerSector = FileFsSize.BytesPerSector;
    }

    if (lpNumberOfFreeClusters != NULL)
    {
        if (!Below2GB)
        {
            *lpNumberOfFreeClusters = FileFsSize.AvailableAllocationUnits.LowPart;
        }
        /* If we have to remain below 2GB... */
        else
        {
            DWORD FreeClusters;

            /* Compute how many clusters there are in less than 2GB: 2 * 1024 * 1024 * 1024- 1 */
            FreeClusters = 0x7FFFFFFF / (FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector);
            /* If that's higher than what was queried, then return the queried value, it's OK! */
            if (FreeClusters > FileFsSize.AvailableAllocationUnits.LowPart)
            {
                FreeClusters = FileFsSize.AvailableAllocationUnits.LowPart;
            }

            *lpNumberOfFreeClusters = FreeClusters;
        }
    }

    if (lpTotalNumberOfClusters != NULL)
    {
        if (!Below2GB)
        {
            *lpTotalNumberOfClusters = FileFsSize.TotalAllocationUnits.LowPart;
        }
        /* If we have to remain below 2GB... */
        else
        {
            DWORD TotalClusters;

            /* Compute how many clusters there are in less than 2GB: 2 * 1024 * 1024 * 1024- 1 */
            TotalClusters = 0x7FFFFFFF / (FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector);
            /* If that's higher than what was queried, then return the queried value, it's OK! */
            if (TotalClusters > FileFsSize.TotalAllocationUnits.LowPart)
            {
                TotalClusters = FileFsSize.TotalAllocationUnits.LowPart;
            }

            *lpTotalNumberOfClusters = TotalClusters;
        }
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDiskFreeSpaceExA(IN LPCSTR lpDirectoryName OPTIONAL,
                    OUT PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                    OUT PULARGE_INTEGER lpTotalNumberOfBytes,
                    OUT PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
    PCSTR RootPath;
    PUNICODE_STRING RootPathU;

    RootPath = lpDirectoryName;
    if (RootPath == NULL)
    {
        RootPath = "\\";
    }

    RootPathU = Basep8BitStringToStaticUnicodeString(RootPath);
    if (RootPathU == NULL)
    {
        return FALSE;
    }

    return GetDiskFreeSpaceExW(RootPathU->Buffer, lpFreeBytesAvailableToCaller,
                              lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetDiskFreeSpaceExW(IN LPCWSTR lpDirectoryName OPTIONAL,
                    OUT PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                    OUT PULARGE_INTEGER lpTotalNumberOfBytes,
                    OUT PULARGE_INTEGER lpTotalNumberOfFreeBytes)
{
    PCWSTR RootPath;
    NTSTATUS Status;
    HANDLE RootHandle;
    UNICODE_STRING FileName;
    DWORD BytesPerAllocationUnit;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_FS_SIZE_INFORMATION FileFsSize;

    /* If no path provided, get root path */
    RootPath = lpDirectoryName;
    if (lpDirectoryName == NULL)
    {
        RootPath = L"\\";
    }

    /* Convert the path to NT path */
    if (!RtlDosPathNameToNtPathName_U(RootPath, &FileName, NULL, NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    /* Open it for disk space query! */
    InitializeObjectAttributes(&ObjectAttributes, &FileName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = NtOpenFile(&RootHandle, SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_FREE_SPACE_QUERY);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        /* If error conversion lead to file not found, override to use path not found
         * which is more accurate
         */
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            SetLastError(ERROR_PATH_NOT_FOUND);
        }

        RtlFreeHeap(RtlGetProcessHeap(), 0, FileName.Buffer);

        return FALSE;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, FileName.Buffer);

    /* If user asks for lpTotalNumberOfFreeBytes, try to use full size information */
    if (lpTotalNumberOfFreeBytes != NULL)
    {
        FILE_FS_FULL_SIZE_INFORMATION FileFsFullSize;

        /* Issue the full fs size request */
        Status = NtQueryVolumeInformationFile(RootHandle, &IoStatusBlock, &FileFsFullSize,
                                              sizeof(FILE_FS_FULL_SIZE_INFORMATION),
                                              FileFsFullSizeInformation);
        /* If it succeed, complete out buffers */
        if (NT_SUCCESS(Status))
        {
            /* We can close here, we'll return */
            NtClose(RootHandle);

            /* Compute the size of an AU */
            BytesPerAllocationUnit = FileFsFullSize.SectorsPerAllocationUnit * FileFsFullSize.BytesPerSector;

            /* And then return what was asked */
            if (lpFreeBytesAvailableToCaller != NULL)
            {
                lpFreeBytesAvailableToCaller->QuadPart = FileFsFullSize.CallerAvailableAllocationUnits.QuadPart * BytesPerAllocationUnit;
            }

            if (lpTotalNumberOfBytes != NULL)
            {
                lpTotalNumberOfBytes->QuadPart = FileFsFullSize.TotalAllocationUnits.QuadPart * BytesPerAllocationUnit;
            }

            /* No need to check for nullness ;-) */
            lpTotalNumberOfFreeBytes->QuadPart = FileFsFullSize.ActualAvailableAllocationUnits.QuadPart * BytesPerAllocationUnit;

            return TRUE;
        }
    }

    /* Otherwise, fallback to normal size information */
    Status = NtQueryVolumeInformationFile(RootHandle, &IoStatusBlock,
                                          &FileFsSize, sizeof(FILE_FS_SIZE_INFORMATION),
                                          FileFsSizeInformation);
    NtClose(RootHandle);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Compute the size of an AU */
    BytesPerAllocationUnit = FileFsSize.SectorsPerAllocationUnit * FileFsSize.BytesPerSector;

    /* And then return what was asked, available is free, the same! */
    if (lpFreeBytesAvailableToCaller != NULL)
    {
        lpFreeBytesAvailableToCaller->QuadPart = FileFsSize.AvailableAllocationUnits.QuadPart * BytesPerAllocationUnit;
    }

    if (lpTotalNumberOfBytes != NULL)
    {
        lpTotalNumberOfBytes->QuadPart = FileFsSize.TotalAllocationUnits.QuadPart * BytesPerAllocationUnit;
    }

    if (lpTotalNumberOfFreeBytes != NULL)
    {
        lpTotalNumberOfFreeBytes->QuadPart = FileFsSize.AvailableAllocationUnits.QuadPart * BytesPerAllocationUnit;
    }

    return TRUE;
}

/*
 * @implemented
 */
UINT
WINAPI
GetDriveTypeA(IN LPCSTR lpRootPathName)
{
    PWCHAR RootPathNameW;

    if (!lpRootPathName)
        return GetDriveTypeW(NULL);

    if (!(RootPathNameW = FilenameA2W(lpRootPathName, FALSE)))
        return DRIVE_UNKNOWN;

    return GetDriveTypeW(RootPathNameW);
}

/*
 * @implemented
 */
UINT
WINAPI
GetDriveTypeW(IN LPCWSTR lpRootPathName)
{
    FILE_FS_DEVICE_INFORMATION FileFsDevice;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING PathName;
    HANDLE FileHandle;
    NTSTATUS Status;
    PWSTR CurrentDir = NULL;
    PCWSTR lpRootPath;

    if (!lpRootPathName)
    {
        /* If NULL is passed, use current directory path */
        DWORD BufferSize = GetCurrentDirectoryW(0, NULL);
        CurrentDir = HeapAlloc(GetProcessHeap(), 0, BufferSize * sizeof(WCHAR));
        if (!CurrentDir)
            return DRIVE_UNKNOWN;
        if (!GetCurrentDirectoryW(BufferSize, CurrentDir))
        {
            HeapFree(GetProcessHeap(), 0, CurrentDir);
            return DRIVE_UNKNOWN;
        }

        if (wcslen(CurrentDir) > 3)
            CurrentDir[3] = 0;

        lpRootPath = CurrentDir;
    }
    else
    {
        size_t Length = wcslen(lpRootPathName);

        TRACE("lpRootPathName: %S\n", lpRootPathName);

        lpRootPath = lpRootPathName;
        if (Length == 2)
        {
            WCHAR DriveLetter = RtlUpcaseUnicodeChar(lpRootPathName[0]);

            if (DriveLetter >= L'A' && DriveLetter <= L'Z' && lpRootPathName[1] == L':')
            {
                Length = (Length + 2) * sizeof(WCHAR);

                CurrentDir = HeapAlloc(GetProcessHeap(), 0, Length);
                if (!CurrentDir)
                    return DRIVE_UNKNOWN;

                StringCbPrintfW(CurrentDir, Length, L"%s\\", lpRootPathName);

                lpRootPath = CurrentDir;
            }
        }
    }

    TRACE("lpRootPath: %S\n", lpRootPath);

    if (!RtlDosPathNameToNtPathName_U(lpRootPath, &PathName, NULL, NULL))
    {
        if (CurrentDir != NULL)
            HeapFree(GetProcessHeap(), 0, CurrentDir);

        return DRIVE_NO_ROOT_DIR;
    }

    TRACE("PathName: %S\n", PathName.Buffer);

    if (CurrentDir != NULL)
        HeapFree(GetProcessHeap(), 0, CurrentDir);

    if (PathName.Buffer[(PathName.Length >> 1) - 1] != L'\\')
    {
        return DRIVE_NO_ROOT_DIR;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &PathName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    RtlFreeHeap(RtlGetProcessHeap(), 0, PathName.Buffer);
    if (!NT_SUCCESS(Status))
        return DRIVE_NO_ROOT_DIR; /* According to WINE regression tests */

    Status = NtQueryVolumeInformationFile(FileHandle,
                                          &IoStatusBlock,
                                          &FileFsDevice,
                                          sizeof(FILE_FS_DEVICE_INFORMATION),
                                          FileFsDeviceInformation);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    switch (FileFsDevice.DeviceType)
    {
        case FILE_DEVICE_CD_ROM:
        case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            return DRIVE_CDROM;
        case FILE_DEVICE_VIRTUAL_DISK:
            return DRIVE_RAMDISK;
        case FILE_DEVICE_NETWORK_FILE_SYSTEM:
            return DRIVE_REMOTE;
        case FILE_DEVICE_DISK:
        case FILE_DEVICE_DISK_FILE_SYSTEM:
            if (FileFsDevice.Characteristics & FILE_REMOTE_DEVICE)
                return DRIVE_REMOTE;
            if (FileFsDevice.Characteristics & FILE_REMOVABLE_MEDIA)
                return DRIVE_REMOVABLE;
        return DRIVE_FIXED;
    }

    ERR("Returning DRIVE_UNKNOWN for device type %lu\n", FileFsDevice.DeviceType);

    return DRIVE_UNKNOWN;
}

/* EOF */
