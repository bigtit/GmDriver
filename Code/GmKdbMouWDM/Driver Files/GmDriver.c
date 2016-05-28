//////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014,My Engine All Rights Reserved.
//
// FileName: GmDriver.c
// Summary: 
//
// Version: 1.0
// Author: Chen Xiao
// Data: 2016年5月20日 12:10
//////////////////////////////////////////////////////////////////////////
#include "Common.h"

// 默认分发函数
NTSTATUS GmKMClassDefaultDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIRP)
{
	// 未使用参数
	(VOID)(pDeviceObject);
	// 这类请求都不支持
	pIRP->IoStatus.Status = STATUS_NOT_SUPPORTED;
	pIRP->IoStatus.Information = 0;
	IoCompleteRequest(pIRP, IO_NO_INCREMENT);
	return STATUS_NOT_SUPPORTED;
}

// 打开分发函数
NTSTATUS GmKMClassCreateDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIRP)
{
	// 未使用参数
	(void)(pDeviceObject);
	// 直接返回成功
	pIRP->IoStatus.Status = STATUS_SUCCESS;
	pIRP->IoStatus.Information = 0;
	IoCompleteRequest(pIRP, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// 关闭分发函数
NTSTATUS GmKMClassCloseDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIRP)
{
	// 未使用参数
	(void)(pDeviceObject);
	// 直接返回成功
	pIRP->IoStatus.Status = STATUS_SUCCESS;
	pIRP->IoStatus.Information = 0;
	IoCompleteRequest(pIRP, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// 控制分发函数，主要接口
NTSTATUS GmKMClassControlDispatch(IN PDEVICE_OBJECT pDeviceObject, IN PIRP pIRP)
{
	PIO_STACK_LOCATION pIRPStack = IoGetCurrentIrpStackLocation(pIRP);
	ControlDeviceExtension* pCtlDeviceExt = (ControlDeviceExtension*)pDeviceObject->DeviceExtension;
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	pIRP->IoStatus.Information = 0;
	PVOID pIOBuffer = pIRP->AssociatedIrp.SystemBuffer;
	if (!pIOBuffer)
	{
		KdPrint(("GmKMClass[控制分发函数]见鬼了，缓冲区是空指针...\n"));
		goto Exit;
	}

	// 请求控制功能
	ULONG nInputConsumed = 0;
	ULONG nCtlCode = pIRPStack->Parameters.DeviceIoControl.IoControlCode;
	if (nCtlCode == KBD_CTL_CODE)
	{
		if (pIRPStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(KEYBOARD_INPUT_DATA))
		{
			KdPrint(("GmKMClass[控制分发函数]给的键盘数据大小不对...\n"));
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
			KdPrint(("GmKMClass[控制分发函数]给的鼠标数据大小不对...\n"));
			nStatus = STATUS_INVALID_PARAMETER;
			goto Exit;
		}
		PMOUSE_INPUT_DATA pMouInput = (PMOUSE_INPUT_DATA)pIOBuffer;
		pCtlDeviceExt->m_pfnMouseClassServiceCallback(pCtlDeviceExt->m_pMouseDeviceObject, pMouInput, pMouInput + 1, &nInputConsumed);
		nStatus = STATUS_SUCCESS;
	}
	else
	{
		KdPrint(("GmKMClass[控制分发函数]出现了未定义的控制代码...\n"));
		nStatus = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

Exit:
	pIRP->IoStatus.Status = nStatus;
	IoCompleteRequest(pIRP, IO_NO_INCREMENT);
	return nStatus;
}

// 驱动卸载
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	// 卸载链接符号
	UNICODE_STRING SymbolicName = RTL_CONSTANT_STRING(KBD_MOU_CTL_DEVICE_SYMBOL_NAME);
	IoDeleteSymbolicLink(&SymbolicName);
	// 卸载控制设备
	IoDeleteDevice(pDriverObject->DeviceObject);
	KdPrint(("GmKMClass驱动卸载成功...\n"));
}

// 驱动入口
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	// 未使用参数
	(VOID)(pRegistryPath);
	NTSTATUS nStatus = STATUS_UNSUCCESSFUL;
	KdPrint(("GmKMClass键盘鼠标模拟驱动开始尝试加载...\n"));
	
	// 设置驱动卸载函数
	pDriverObject->DriverUnload = DriverUnload;

	// 设置分发函数
	for (INT i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		pDriverObject->MajorFunction[i] = GmKMClassDefaultDispatch;
	}

	// 覆盖一些特别的分发函数
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = GmKMClassCreateDispatch;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = GmKMClassCloseDispatch;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = GmKMClassControlDispatch;

	// 创建控制设备对象
	UNICODE_STRING ControlDeviceName = RTL_CONSTANT_STRING(KBD_MOU_CTL_DEVICE_OBJECT_NAME);
	PDEVICE_OBJECT pControlDeviceObject = NULL;
	nStatus = IoCreateDevice(pDriverObject, sizeof(ControlDeviceExtension), &ControlDeviceName, CTL_DEVICE_TYPE, 0, FALSE, &pControlDeviceObject);
	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("GmKMClass创建控制设备对象失败，返回错误码 = 0x%08X\n", nStatus));
		return nStatus;
	}
	// 设置域
	pControlDeviceObject->Flags |= DO_DIRECT_IO;
	// 对齐方式
	pControlDeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;

	// 搜索键盘回调函数
	nStatus = SearchKeyboardClassServiceCallbackAddress(pControlDeviceObject);
	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("GmKMClass没有找到键盘的回调函数...\n"));
		IoDeleteDevice(pControlDeviceObject);
		pControlDeviceObject = NULL;
		return nStatus;
	}

	// 搜索鼠标回调函数
	nStatus = SearchMouseClassServiceCallback(pControlDeviceObject);
	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("GmKMClass没有找到鼠标的回调函数...\n"));
		IoDeleteDevice(pControlDeviceObject);
		pControlDeviceObject = NULL;
		return nStatus;
	}

	// 创建链接符号，暴露给应用层
	UNICODE_STRING SymbolicName = RTL_CONSTANT_STRING(KBD_MOU_CTL_DEVICE_SYMBOL_NAME);
	nStatus = IoCreateSymbolicLink(&SymbolicName, &ControlDeviceName);
	if (!NT_SUCCESS(nStatus))
	{
		KdPrint(("GmKMClass创建链接符号失败，返回错误码 = 0x%08X...\n", nStatus));
		IoDeleteDevice(pControlDeviceObject);
		pControlDeviceObject = NULL;
		return nStatus;
	}

	// 设置初始化域
	KdPrint(("GmKMClass驱动加载成功！\n"));
	pControlDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	return STATUS_SUCCESS;
}

