//////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014,My Engine All Rights Reserved.
//
// FileName: Common.cpp
// Summary: 
//
// Version: 1.0
// Author: Chen Xiao
// Data: 2016��5��20�� 12:20
//////////////////////////////////////////////////////////////////////////
#include "Common.h"

PDRIVER_OBJECT GetDriverObject(IN PUNICODE_STRING pDriverName)
{
	PDRIVER_OBJECT pDriverObject = NULL;
	NTSTATUS nStatus = ObReferenceObjectByName(pDriverName, OBJ_CASE_INSENSITIVE, NULL, FILE_ANY_ACCESS, *IoDriverObjectType, KernelMode, NULL, (PVOID*)(&pDriverObject));
	// �ɹ��򿪣��������
	if (NT_SUCCESS(nStatus))
	{
		ObDereferenceObject(pDriverObject);
	}
	else
	{
		KdPrint(("Can't Get Driver Object...DriverName = %wZ, nStatus = %ld", *pDriverName, nStatus));
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
	// �ȴ򿪶˿ڲ��PS/2������������
	UNICODE_STRING I8042PrtDriverName = RTL_CONSTANT_STRING(PS2_KBD_PORT_DRIVER_NAME);
	PDRIVER_OBJECT pI8042PrtDriver = GetDriverObject(&I8042PrtDriverName);
	// �ٳ��Դ򿪶˿ڲ�USB������������
	UNICODE_STRING KbdHIDDriverName = RTL_CONSTANT_STRING(USB_KBD_PORT_DRIVER_NAME);
	PDRIVER_OBJECT pKbdHIDDriver = GetDriverObject(&KbdHIDDriverName);
	// ����������̶��ҵ��ˣ�����USB����
	PDRIVER_OBJECT pUsingPortDriver = pKbdHIDDriver ? pKbdHIDDriver : pI8042PrtDriver;
	// ���һ�����̶�û���ҵ�
	if (!pUsingPortDriver)
	{
		KdPrint(("There Is No Any Keyboard..."));
		return STATUS_UNSUCCESSFUL;
	}

	// ���豸������������
	UNICODE_STRING ClassDriverName = RTL_CONSTANT_STRING(KBD_CLASS_DRIVER_NAME);
	PDRIVER_OBJECT pClassDriver = GetDriverObject(&ClassDriverName);
	if (!pClassDriver)
	{
		KdPrint(("Can't Not Get The Keyboard Class Driver Object..."));
		return STATUS_UNSUCCESSFUL;
	}

	// ��ʼ�����˿����������µ������豸������ĳ���豸������������ϲ����������豸����Ļص�������Ҳ����KeyboardClassServiceCallback��
	PVOID pClassDriverStart = pClassDriver->DriverStart;
	ULONG nClassDriverSize = pClassDriver->DriverSize;
	ControlDeviceExtension* pControlDeviceExtension = (ControlDeviceExtension*)pControlDeviceObject->DeviceExtension;
	for (PDEVICE_OBJECT pCurPortDevice = pUsingPortDriver->DeviceObject; pCurPortDevice; pCurPortDevice = pCurPortDevice->NextDevice)
	{
		// һ���������ָ��Ӧ�ñ�����i8042prt���ɵ��豸���Զ�����չ��
		// ������������Ŀ�ʼ��ַӦ�����ں�ģ��KbdClasss��
		// �����ں�ģ��KbcClass���ɵ�һ���豸�����ָ��Ҳ�����ڶ˿��������Զ�����չ�У��������������ָ��֮ǰ
		// ʵ��Debug���֣�����ֻ������PS/2���̣�ȷʵ�Ǳ�����i8042prt���ɵ��豸���Զ�����չ��
		// ʵ�ʺ���ָ����豸����ָ��Ӧ�ñ�����KbdClasss���豸�����������豸ջ����һ���豸������Զ�����չ��
		// USB��PS/2�������ǣ�USB�������i8042prt��KbdClasss���豸ջ�У���������һ���豸���󣬶�����ָ��Ҳ���Ǳ���������豸�����У����KbdClasss�豸����

		// ����������Ӧ�����ҵ�����˿��豸������豸ջ�����KbdClasss�豸������豸����
		PDEVICE_OBJECT pLowerDevice = pCurPortDevice;
		while (pLowerDevice->AttachedDevice)
		{
			if (RtlCompareUnicodeString(&pLowerDevice->AttachedDevice->DriverObject->DriverName, &ClassDriverName, TRUE) == 0)
			{
				KdPrint(("Find The Keyboard Target Port Device Object..."));
				break;
			}
			pLowerDevice = pLowerDevice->AttachedDevice;
		}

		// �Ѿ��ҵ��豸ջ���ˣ�Ҳû���ҵ����KbdClasss�豸������豸����
		if (!pLowerDevice->AttachedDevice)
		{
			KdPrint(("Can't Find The Keyboard Target Port Device Object..."));
			continue;
		}
		
		// ��ʼ��pLowerDevice���Զ�����չ���һص��������豸����ָ��
		for (PDEVICE_OBJECT pCurClassDevice = pClassDriver->DeviceObject; pCurClassDevice; pCurClassDevice = pCurClassDevice->NextDevice)
		{
			// �����
			pControlDeviceExtension->m_pKeyboradDeviceObject = NULL;
			pControlDeviceExtension->m_pfnKeyboardClassServiceCallback = NULL;
			// ��������豸������Զ�����չ���ݣ�������Ҫ�Ķ������������汣����
			PCHAR pDeviceExtension = (PCHAR)pLowerDevice->DeviceExtension;
			for (INT i = 0; i < 4096; ++i, pDeviceExtension += sizeof(PVOID))
			{
				// �ڴ治�Ϸ�
				if (!MmIsAddressValid(pDeviceExtension))
				{
					KdPrint(("The Address Is InValid...pDeviceExtension = %p", pDeviceExtension));
					break;
				}

				// ����һ��ָ��ȡֵ
				PVOID pTemPtr = *(PVOID*)pDeviceExtension;
				// �ҵ�һ����ַ�������ַ������һ��ָ�룬һ������KbdClasss����������豸����ָ��
				if (pTemPtr == pCurClassDevice)
				{
					pControlDeviceExtension->m_pKeyboradDeviceObject = pTemPtr;
					continue;
				}

				// ���ҵ�KbdClasss�������豸�������Һ���ĺ���ָ�룬�ҵ����豸ָ����ҵ�һ����ַ�������ַ������һ������ָ�룬һ��λ��KbdClasss����������ڴ�ռ����ָ��
				if (pControlDeviceExtension->m_pKeyboradDeviceObject && IsInAddress(pTemPtr, pClassDriverStart, nClassDriverSize) && MmIsAddressValid(pTemPtr))
				{
					pControlDeviceExtension->m_pfnKeyboardClassServiceCallback = *((KeyboardClassServiceCallback*)pDeviceExtension);
					return STATUS_SUCCESS;
				}
			} // �����չ��ַû������ƫ��һ��ָ��������
		} // ���KbdClass���豸����û�б�pLowerDevice���Զ�����չ�����棬���Բ�����Ҫ���Ǹ��豸������һ��KbdClass�豸����
	} // ���pLowerDevice�豸�����û�б����κ�KbdClass�豸������һ���˿��豸���������pLowerDevice

	return STATUS_UNSUCCESSFUL;
}

NTSTATUS SearchMouseClassServiceCallback(IN PDEVICE_OBJECT pControlDeviceObject)
{
	// �ȴ򿪶˿ڲ��PS/2������������
	UNICODE_STRING I8042PrtDriverName = RTL_CONSTANT_STRING(PS2_MOU_PORT_DRIVER_NAME);
	PDRIVER_OBJECT pI8042PrtDriver = GetDriverObject(&I8042PrtDriverName);
	// �ٳ��Դ򿪶˿ڲ�USB������������
	UNICODE_STRING KbdHIDDriverName = RTL_CONSTANT_STRING(USB_MOU_PORT_DRIVER_NAME);
	PDRIVER_OBJECT pKbdHIDDriver = GetDriverObject(&KbdHIDDriverName);
	// ����������̶��ҵ��ˣ�����USB����
	PDRIVER_OBJECT pUsingPortDriver = pKbdHIDDriver ? pKbdHIDDriver : pI8042PrtDriver;
	// ���һ����궼û���ҵ�
	if (!pUsingPortDriver)
	{
		KdPrint(("There Is No Any Mouse..."));
		return STATUS_UNSUCCESSFUL;
	}

	// ���豸������������
	UNICODE_STRING ClassDriverName = RTL_CONSTANT_STRING(MOU_CLASS_DRIVER_NAME);
	PDRIVER_OBJECT pClassDriver = GetDriverObject(&ClassDriverName);
	if (!pClassDriver)
	{
		KdPrint(("Can't Not Get The Mouse Class Driver Object..."));
		return STATUS_UNSUCCESSFUL;
	}

	// ��ʼ�����˿����������µ������豸������ĳ���豸������������ϲ����������豸����Ļص�������Ҳ����MouseClassServiceCallback��
	PVOID pClassDriverStart = pClassDriver->DriverStart;
	ULONG nClassDriverSize = pClassDriver->DriverSize;
	ControlDeviceExtension* pControlDeviceExtension = (ControlDeviceExtension*)pControlDeviceObject->DeviceExtension;
	for (PDEVICE_OBJECT pCurPortDevice = pUsingPortDriver->DeviceObject; pCurPortDevice; pCurPortDevice = pCurPortDevice->NextDevice)
	{
		// һ���������ָ��Ӧ�ñ�����i8042prt���ɵ��豸���Զ�����չ��
		// ������������Ŀ�ʼ��ַӦ�����ں�ģ��MouClass��
		// �����ں�ģ��MouClass���ɵ�һ���豸�����ָ��Ҳ�����ڶ˿��������Զ�����չ�У��������������ָ��֮ǰ
		// ʵ��Debug���֣�����ֻ������PS/2��꣬ȷʵ�Ǳ�����i8042prt���ɵ��豸���Զ�����չ��
		// ʵ�ʺ���ָ����豸����ָ��Ӧ�ñ�����MouClass���豸�����������豸ջ����һ���豸������Զ�����չ��
		// USB��PS/2�������ǣ�USB�������i8042prt��MouClass���豸ջ�У���������һ���豸���󣬶�����ָ��Ҳ���Ǳ���������豸�����У����MouClass�豸����

		// ����������Ӧ�����ҵ�����˿��豸������豸ջ�����MouClass�豸������豸����
		PDEVICE_OBJECT pLowerDevice = pCurPortDevice;
		while (pLowerDevice->AttachedDevice)
		{
			if (RtlCompareUnicodeString(&pLowerDevice->AttachedDevice->DriverObject->DriverName, &ClassDriverName, TRUE) == 0)
			{
				KdPrint(("Find The Mouse Target Port Device Object..."));
				break;
			}
			pLowerDevice = pLowerDevice->AttachedDevice;
		}

		// �Ѿ��ҵ��豸ջ���ˣ�Ҳû���ҵ����KbdClasss�豸������豸����
		if (!pLowerDevice->AttachedDevice)
		{
			KdPrint(("Can't Find Mouse The Target Port Device Object..."));
			continue;
		}

		// ��ʼ��pLowerDevice���Զ�����չ���һص��������豸����ָ��
		for (PDEVICE_OBJECT pCurClassDevice = pClassDriver->DeviceObject; pCurClassDevice; pCurClassDevice = pCurClassDevice->NextDevice)
		{
			// �����
			pControlDeviceExtension->m_pMouseDeviceObject = NULL;
			pControlDeviceExtension->m_pfnMouseClassServiceCallback = NULL;
			// ��������豸������Զ�����չ���ݣ�������Ҫ�Ķ������������汣����
			PCHAR pDeviceExtension = (PCHAR)pLowerDevice->DeviceExtension;
			for (INT i = 0; i < 4096; ++i, pDeviceExtension += sizeof(PVOID))
			{
				// �ڴ治�Ϸ�
				if (!MmIsAddressValid(pDeviceExtension))
				{
					KdPrint(("The Address Is InValid...pDeviceExtension = %p", pDeviceExtension));
					break;
				}

				// ����һ��ָ��ȡֵ
				PVOID pTemPtr = *(PVOID*)pDeviceExtension;
				// �ҵ�һ����ַ�������ַ������һ��ָ�룬һ������KbdClasss����������豸����ָ��
				if (pTemPtr == pCurClassDevice)
				{
					pControlDeviceExtension->m_pMouseDeviceObject = pCurClassDevice;
					continue;
				}

				// ���ҵ�MouClass�������豸�������Һ���ĺ���ָ�룬�ҵ����豸ָ����ҵ�һ����ַ�������ַ������һ������ָ�룬һ��λ��MouClass����������ڴ�ռ����ָ��
				if (pControlDeviceExtension->m_pMouseDeviceObject && IsInAddress(pTemPtr, pClassDriverStart, nClassDriverSize) && MmIsAddressValid(pTemPtr))
				{
					pControlDeviceExtension->m_pfnMouseClassServiceCallback = *((MouseClassServiceCallback*)pDeviceExtension);
					return STATUS_SUCCESS;
				}
			} // �����չ��ַû������ƫ��һ��ָ��������
		} // ���MouClass���豸����û�б�pLowerDevice���Զ�����չ�����棬���Բ�����Ҫ���Ǹ��豸������һ��MouClass�豸����
	} // ���pLowerDevice�豸�����û�б����κ�MouClass�豸������һ���˿��豸���������pLowerDevice

	return STATUS_UNSUCCESSFUL;
}
