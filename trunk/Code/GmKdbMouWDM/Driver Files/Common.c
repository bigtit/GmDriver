//////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014,My Engine All Rights Reserved.
//
// FileName: Common.cpp
// Summary: 
//
// Version: 1.0
// Author: Chen Xiao
// Data: 2016年5月20日 12:20
//////////////////////////////////////////////////////////////////////////
#include "Common.h"

PDRIVER_OBJECT GetDriverObject(IN PUNICODE_STRING pDriverName)
{
	PDRIVER_OBJECT pDriverObject = NULL;
	NTSTATUS nStatus = ObReferenceObjectByName(pDriverName, OBJ_CASE_INSENSITIVE, NULL, FILE_ANY_ACCESS, *IoDriverObjectType, KernelMode, NULL, (PVOID*)(&pDriverObject));
	// 成功打开，解除引用
	if (NT_SUCCESS(nStatus))
	{
		ObDereferenceObject(pDriverObject);
	}
	else
	{
		KdPrint(("GmKMClass无法打开[%wZ]驱动对象， 返回错误码 = 0x%08X", *pDriverName, nStatus));
	}

	return pDriverObject;
}

INT IsInAddress(PVOID pTarget, PVOID pStart, ULONG nSize)
{
	if (!pTarget || !pStart || nSize <= 0)
	{
		return FALSE;
	}

	if (pTarget >= pStart && pTarget < (PVOID)((PCHAR)pTarget + nSize))
	{
		return TRUE;
	}

	return FALSE;
}

NTSTATUS SearchKeyboardClassServiceCallbackAddress(IN PDEVICE_OBJECT pControlDeviceObject)
{
	// 先打开端口层的PS/2键盘驱动对象
	UNICODE_STRING I8042PrtDriverName = RTL_CONSTANT_STRING(PS2_KBD_PORT_DRIVER_NAME);
	PDRIVER_OBJECT pI8042PrtDriver = GetDriverObject(&I8042PrtDriverName);
	// 再尝试打开端口层USB键盘驱动对象
	UNICODE_STRING KbdHIDDriverName = RTL_CONSTANT_STRING(USB_KBD_PORT_DRIVER_NAME);
	PDRIVER_OBJECT pKbdHIDDriver = GetDriverObject(&KbdHIDDriverName);
	// 如果两个键盘都找到了，则用USB键盘
	PDRIVER_OBJECT pUsingPortDriver = pKbdHIDDriver ? pKbdHIDDriver : pI8042PrtDriver;
	// 如果一个键盘都没有找到
	if (!pUsingPortDriver)
	{
		KdPrint(("GmKMClass[键盘]没有找到任何键盘设备...\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// 打开设备类层的驱动对象
	UNICODE_STRING ClassDriverName = RTL_CONSTANT_STRING(KBD_CLASS_DRIVER_NAME);
	PDRIVER_OBJECT pClassDriver = GetDriverObject(&ClassDriverName);
	if (!pClassDriver)
	{
		KdPrint(("GmKMClass[键盘]打开KbdClasss驱动对象失败...\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// 开始遍历端口驱动对象下的所有设备对象（在某个设备对象里面存有上层类驱动的设备对象的回调函数，也就是KeyboardClassServiceCallback）
	PVOID pClassDriverStart = pClassDriver->DriverStart;
	ULONG nClassDriverSize = pClassDriver->DriverSize;
	ControlDeviceExtension* pControlDeviceExtension = (ControlDeviceExtension*)pControlDeviceObject->DeviceExtension;
	for (PDEVICE_OBJECT pCurPortDevice = pUsingPortDriver->DeviceObject; pCurPortDevice; pCurPortDevice = pCurPortDevice->NextDevice)
	{
		// 一、这个函数指针应该保存在i8042prt生成的设备的自定义扩展中
		// 二、这个函数的开始地址应该在内核模块KbdClasss中
		// 三、内核模块KbcClass生成的一个设备对象的指针也保存在端口驱动的自定义扩展中，并且在这个函数指针之前
		// 实际Debug发现，上述只适用于PS/2键盘，确实是保存在i8042prt生成的设备的自定义扩展中
		// 实际函数指针和设备对象指针应该保存在KbdClasss的设备对象所处的设备栈的下一层设备对象的自定义扩展中
		// USB与PS/2的区别是，USB情况下在i8042prt和KbdClasss的设备栈中，还有另外一层设备对象，而函数指针也就是保存在这个设备对象中，最靠近KbdClasss设备对象

		// 根据上述，应该先找到最靠近端口设备对象的设备栈中最靠近KbdClasss设备对象的设备对象
		PDEVICE_OBJECT pLowerDevice = pCurPortDevice;
		while (pLowerDevice->AttachedDevice)
		{
			KdPrint(("GmKMClasss[键盘]开始搜索[%wZ]第[%d]个设备栈上的[%wZ]设备对象...", pCurPortDevice->DriverObject->DriverName, pCurPortDevice->StackSize, pLowerDevice->DriverObject->DriverName));
			for (PDEVICE_OBJECT pCurClassDevice = pClassDriver->DeviceObject; pCurClassDevice; pCurClassDevice = pCurClassDevice->NextDevice)
			{
				// 先清空
				pControlDeviceExtension->m_pKeyboradDeviceObject = NULL;
				pControlDeviceExtension->m_pfnKeyboardClassServiceCallback = NULL;
				// 遍历这个设备对象的自定义扩展内容，我们需要的东西就在这里面保存着
				PCHAR pDeviceExtension = (PCHAR)pLowerDevice->DeviceExtension;
				for (INT i = 0; i < 4096; ++i, pDeviceExtension += sizeof(PVOID))
				{
					// 内存不合法
					if (!MmIsAddressValid(pDeviceExtension))
					{
						KdPrint(("GmKMClass[键盘]这个地址[%p]不合法...\n", pDeviceExtension));
						break;
					}

					// 找到一个地址，这个地址保存着一个指针，一个属于KbdClasss驱动对象的设备对象指针
					PDEVICE_OBJECT pTempDevice = *(PVOID*)pDeviceExtension;
					if (pTempDevice != pCurClassDevice)
					{
						continue;
					}

					PVOID pTempCallback = *((PVOID*)(pDeviceExtension + sizeof(PVOID)));
					if (IsInAddress(pTempCallback, pClassDriverStart, nClassDriverSize) && MmIsAddressValid(pTempCallback))
					{
						KdPrint(("GmKMClass[键盘]找到[%p]保存着[%wZ]设备对象[%p]和回调函数[%p]...\n", pDeviceExtension, pCurClassDevice->DriverObject->DriverName, pTempDevice, pTempCallback));
						pControlDeviceExtension->m_pKeyboradDeviceObject = pCurClassDevice;
						pControlDeviceExtension->m_pfnKeyboardClassServiceCallback = *((KeyboardClassServiceCallback*)(pDeviceExtension + sizeof(PVOID)));
						return STATUS_SUCCESS;
					}

					KdPrint(("GmKMClass[键盘]找到[%p]保存的[%wZ]设备对象，但是后面不是回调函数...\n", pDeviceExtension, pCurClassDevice->DriverObject->DriverName));
				}
			} // 这个扩展地址没有往下偏移一个指针再试试
			pLowerDevice = pLowerDevice->AttachedDevice;
		} // 这个KbdClass的设备对象没有被pLowerDevice的自定义扩展所保存，所以不是想要的那个设备对象，下一个KbdClass设备对象
	} // 这个pLowerDevice设备对象就没有保存任何KbdClass设备对象，下一个端口设备对象继续找pLowerDevice

	
	KdPrint(("GmKMClass[键盘]端口驱动的所有设备所处的设备栈上都找不到合适的设备...\n"));
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS SearchMouseClassServiceCallback(IN PDEVICE_OBJECT pControlDeviceObject)
{
	// 先打开端口层的PS/2键盘驱动对象
	UNICODE_STRING I8042PrtDriverName = RTL_CONSTANT_STRING(PS2_MOU_PORT_DRIVER_NAME);
	PDRIVER_OBJECT pI8042PrtDriver = GetDriverObject(&I8042PrtDriverName);
	// 再尝试打开端口层USB键盘驱动对象
	UNICODE_STRING KbdHIDDriverName = RTL_CONSTANT_STRING(USB_MOU_PORT_DRIVER_NAME);
	PDRIVER_OBJECT pKbdHIDDriver = GetDriverObject(&KbdHIDDriverName);
	// 如果两个键盘都找到了，则用USB键盘
	PDRIVER_OBJECT pUsingPortDriver = pKbdHIDDriver ? pKbdHIDDriver : pI8042PrtDriver;
	// 如果一个鼠标都没有找到
	if (!pUsingPortDriver)
	{
		KdPrint(("GmKMClass[鼠标]没有找到任何鼠标设备...\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// 打开设备类层的驱动对象
	UNICODE_STRING ClassDriverName = RTL_CONSTANT_STRING(MOU_CLASS_DRIVER_NAME);
	PDRIVER_OBJECT pClassDriver = GetDriverObject(&ClassDriverName);
	if (!pClassDriver)
	{
		KdPrint(("GmKMClass[鼠标]无法打开MouClasss驱动对象...\n"));
		return STATUS_UNSUCCESSFUL;
	}

	// 开始遍历端口驱动对象下的所有设备对象（在某个设备对象里面存有上层类驱动的设备对象的回调函数，也就是MouseClassServiceCallback）
	PVOID pClassDriverStart = pClassDriver->DriverStart;
	ULONG nClassDriverSize = pClassDriver->DriverSize;
	ControlDeviceExtension* pControlDeviceExtension = (ControlDeviceExtension*)pControlDeviceObject->DeviceExtension;
	for (PDEVICE_OBJECT pCurPortDevice = pUsingPortDriver->DeviceObject; pCurPortDevice; pCurPortDevice = pCurPortDevice->NextDevice)
	{
		// 一、这个函数指针应该保存在i8042prt生成的设备的自定义扩展中
		// 二、这个函数的开始地址应该在内核模块MouClass中
		// 三、内核模块MouClass生成的一个设备对象的指针也保存在端口驱动的自定义扩展中，并且在这个函数指针之前
		// 实际Debug发现，上述只适用于PS/2鼠标，确实是保存在i8042prt生成的设备的自定义扩展中
		// 实际函数指针和设备对象指针应该保存在MouClass的设备对象所处的设备栈的下一层设备对象的自定义扩展中
		// USB与PS/2的区别是，USB情况下在i8042prt和MouClass的设备栈中，还有另外一层设备对象，而函数指针也就是保存在这个设备对象中，最靠近MouClass设备对象

		// 根据上述，应该先找到最靠近端口设备对象的设备栈中最靠近MouClass设备对象的设备对象
		PDEVICE_OBJECT pLowerDevice = pCurPortDevice;
		while (pLowerDevice->AttachedDevice)
		{
			if (RtlCompareUnicodeString(&pLowerDevice->AttachedDevice->DriverObject->DriverName, &ClassDriverName, TRUE) == 0)
			{
				KdPrint(("GmKMClass[鼠标]找到最靠近MouClasss驱动的设备对象[%wZ]...\n", pLowerDevice->DriverObject->DriverName));
				break;
			}
			pLowerDevice = pLowerDevice->AttachedDevice;
		}

		// 已经找到设备栈顶了，也没有找到最靠近KbdClasss设备对象的设备对象
		if (!pLowerDevice->AttachedDevice)
		{
			KdPrint(("GmKMClass[鼠标]这个设备[当前%wZ][栈顶%wZ]所处的设备栈里没有找到合适的设备对象...\n", pCurPortDevice->DriverObject->DriverName, pLowerDevice->DriverObject->DriverName));
			continue;
		}

		// 开始在pLowerDevice的自定义扩展中找回调函数和设备对象指针
		for (PDEVICE_OBJECT pCurClassDevice = pClassDriver->DeviceObject; pCurClassDevice; pCurClassDevice = pCurClassDevice->NextDevice)
		{
			// 先清空
			pControlDeviceExtension->m_pMouseDeviceObject = NULL;
			pControlDeviceExtension->m_pfnMouseClassServiceCallback = NULL;
			// 遍历这个设备对象的自定义扩展内容，我们需要的东西就在这里面保存着
			PCHAR pDeviceExtension = (PCHAR)pLowerDevice->DeviceExtension;
			for (INT i = 0; i < 4096; ++i, pDeviceExtension += sizeof(PVOID))
			{
				// 内存不合法
				if (!MmIsAddressValid(pDeviceExtension))
				{
					KdPrint(("GmKMClass[鼠标]这个地址[%p]不合法...\n", pDeviceExtension));
					break;
				}

				// 找到一个地址，这个地址保存着一个指针，一个属于KbdClasss驱动对象的设备对象指针
				PDEVICE_OBJECT pTempDevice = *(PVOID*)pDeviceExtension;
				if (pTempDevice != pCurClassDevice)
				{
					continue;
				}

				PVOID pTempCallback = *((PVOID*)(pDeviceExtension + sizeof(PVOID)));
				if (IsInAddress(pTempCallback, pClassDriverStart, nClassDriverSize) && MmIsAddressValid(pTempCallback))
				{
					KdPrint(("GmKMClass[鼠标]找到[%p]保存着[%wZ]设备对象[%p]和回调函数[%p]...\n", pDeviceExtension, pCurClassDevice->DriverObject->DriverName, pTempDevice, pTempCallback));
					pControlDeviceExtension->m_pMouseDeviceObject = pCurClassDevice;
					pControlDeviceExtension->m_pfnMouseClassServiceCallback = *((MouseClassServiceCallback*)(pDeviceExtension + sizeof(PVOID)));
					return STATUS_SUCCESS;
				}

				KdPrint(("GmKMClass[鼠标]找到[%p]保存的[%wZ]设备对象，但是后面不是回调函数...\n", pDeviceExtension, pCurClassDevice->DriverObject->DriverName));
			} // 这个扩展地址没有往下偏移一个指针再试试
		} // 这个MouClass的设备对象没有被pLowerDevice的自定义扩展所保存，所以不是想要的那个设备对象，下一个MouClass设备对象
	} // 这个pLowerDevice设备对象就没有保存任何MouClass设备对象，下一个端口设备对象继续找pLowerDevice

	KdPrint(("GmKMClass[鼠标]端口驱动的所有设备所处的设备栈上都找不到合适的设备...\n"));
	return STATUS_UNSUCCESSFUL;
}
