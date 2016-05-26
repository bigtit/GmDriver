//////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014,My Engine All Rights Reserved.
//
// FileName: GmDriver.c
// Summary: 
//
// Version: 1.0
// Author: Chen Xiao
// Data: 2016��5��20�� 12:10
//////////////////////////////////////////////////////////////////////////
#include "Common.h"

// Ĭ�Ϸַ�����
NTSTATUS GmKMClassDefaultDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIRP)
{
	// δʹ�ò���
	(VOID)(pDeviceObject);
	// �������󶼲�֧��
	pIRP->IoStatus.Status = STATUS_NOT_SUPPORTED;
	pIRP->IoStatus.Information = 0;
	IoCompleteRequest(pIRP, IO_NO_INCREMENT);
	return STATUS_NOT_SUPPORTED;
}

// �򿪷ַ�����
NTSTATUS GmKMClassCreateDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIRP)
{
	// δʹ�ò���
	(void)(pDeviceObject);
	// ֱ�ӷ��سɹ�
	pIRP->IoStatus.Status = STATUS_SUCCESS;
	pIRP->IoStatus.Information = 0;
	IoCompleteRequest(pIRP, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// �رշַ�����
NTSTATUS GmKMClassCloseDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIRP)
{
	// δʹ�ò���
	(void)(pDeviceObject);
	// ֱ�ӷ��سɹ�
	pIRP->IoStatus.Status = STATUS_SUCCESS;
	pIRP->IoStatus.Information = 0;
	IoCompleteRequest(pIRP, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// ���Ʒַ���������Ҫ�ӿ�
NTSTATUS GmKMClassControlDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIRP)
{
	PIO_STACK_LOCATION pIRPStack = IoGetCurrentIrpStackLocation(pIRP);
	ControlDeviceExtension* pCtlDeviceExt = (ControlDeviceExtension*)pDeviceObject->DeviceExtension;
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	pIRP->IoStatus.Information = 0;
	PVOID pIOBuffer = pIRP->AssociatedIrp.SystemBuffer;
	if (!pIOBuffer)
	{
		KdPrint(("WTF?! The IOBuffer Is NULL?!"));
		goto Exit;
	}

	// ������ƹ���
	ULONG nInputConsumed = 0;
	ULONG nCtlCode = pIRPStack->Parameters.DeviceIoControl.IoControlCode;
	if (nCtlCode == KBD_CTL_CODE)
	{
		if (pIRPStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(KEYBOARD_INPUT_DATA))
		{
			KdPrint(("The Keyboard Input Data Size Is Not Equal sizeof(KEYBOARD_INPUT_DATA)..."));
			nStatus = STATUS_INVALID_PARAMETER;
			goto Exit;
		}
		PKEYBOARD_INPUT_DATA pKbdInput = (PKEYBOARD_INPUT_DATA)pIOBuffer;
		pCtlDeviceExt->m_pfnKeyboardClassServiceCallback(pCtlDeviceExt->m_pKeyboradDeviceObject, pKbdInput, pKbdInput + 1, &nInputConsumed);
		nStatus = STATUS_SUCCESS;
	}
	else if (nCtlCode == MOU_CTL_CODE)
	{
		if (pIRPStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(MOUSE_INPUT_DATA))
		{
			KdPrint(("The Mouse Input Data Size Is Not Equal sizeof(MOUSE_INPUT_DATA)..."));
			nStatus = STATUS_INVALID_PARAMETER;
			goto Exit;
		}
		PMOUSE_INPUT_DATA pMouInput = (PMOUSE_INPUT_DATA)pIOBuffer;
		pCtlDeviceExt->m_pfnMouseClassServiceCallback(pCtlDeviceExt->m_pMouseDeviceObject, pMouInput, pMouInput + 1, &nInputConsumed);
		nStatus = STATUS_SUCCESS;
	}
	else
	{
		KdPrint(("Are You Fuking Kidding Me?..."));
		nStatus = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

Exit:
	pIRP->IoStatus.Status = nStatus;
	IoCompleteRequest(pIRP, IO_NO_INCREMENT);
	return nStatus;
}

// ����ж��
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	// ж�����ӷ���
	UNICODE_STRING SymbolicName = RTL_CONSTANT_STRING(KBD_MOU_CTL_DEVICE_SYMBOL_NAME);
	IoDeleteSymbolicLink(&SymbolicName);
	// ж�ؿ����豸
	IoDeleteDevice(pDriverObject->DeviceObject);
	KdPrint(("GmKMClass Driver Unload Success..."));
}

// �������
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	// δʹ�ò���
	(VOID)(pRegistryPath);
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	KdPrint(("Driver Entry..."));
	
	// ��������ж�غ���
	pDriverObject->DriverUnload = DriverUnload;

	// ���÷ַ�����
	for (INT i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = GmKMClassDefaultDispatch;
	}

	// ����һЩ�ر�ķַ�����
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = GmKMClassCreateDispatch;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = GmKMClassCloseDispatch;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = GmKMClassControlDispatch;

	// ���������豸����
	UNICODE_STRING ControlDeviceName = RTL_CONSTANT_STRING(KBD_MOU_CTL_DEVICE_OBJECT_NAME);
	PDEVICE_OBJECT pControlDeviceObject = NULL;
	nStatus = IoCreateDevice(pDriverObject, sizeof(ControlDeviceExtension), &ControlDeviceName, CTL_DEVICE_TYPE, 0, FALSE, &pControlDeviceObject);
	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("IoCreateDevice Fail...Status = %ld", nStatus));
		return nStatus;
	}
	// ������
	pControlDeviceObject->Flags |= DO_DIRECT_IO;
	// ���뷽ʽ
	pControlDeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;

	// �������̻ص�����
	nStatus = SearchKeyboardClassServiceCallbackAddress(pControlDeviceObject);
	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("Can't Find The Keyboard Callback..."));
		IoDeleteDevice(pControlDeviceObject);
		pControlDeviceObject = NULL;
		return nStatus;
	}

	// �������ص�����
	nStatus = SearchMouseClassServiceCallback(pControlDeviceObject);
	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("Can't Find The Mouse Callback..."));
		IoDeleteDevice(pControlDeviceObject);
		pControlDeviceObject = NULL;
		return nStatus;
	}

	// �������ӷ��ţ���¶��Ӧ�ò�
	UNICODE_STRING SymbolicName = RTL_CONSTANT_STRING(KBD_MOU_CTL_DEVICE_SYMBOL_NAME);
	nStatus = IoCreateSymbolicLink(&SymbolicName, &ControlDeviceName);
	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("Can't Create The Symbolic Link..."));
		IoDeleteDevice(pControlDeviceObject);
		pControlDeviceObject = NULL;
		return nStatus;
	}

	// ���ó�ʼ����
	KdPrint(("Driver Load Success..."));
	pControlDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	return STATUS_SUCCESS;
}

