//////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014,My Engine All Rights Reserved.
//
// FileName: Common.h
// Summary: 
//
// Version: 1.0
// Author: Chen Xiao
// Data: 2016年5月20日 18:24
//////////////////////////////////////////////////////////////////////////
#ifndef __Common_H__
#define __Common_H__
#include <ntddk.h>
#include <ntddkbd.h>
#include <ntddmou.h>

// 在键盘设备栈上，由KbdClasss驱动对象上生成的设备对象的键盘回调函数，该回调函数的函数指针在下层的设备对象上存有一份（i8042prt或者kbdhid驱动对象的设备）
typedef VOID (__stdcall *KeyboardClassServiceCallback)(IN PDEVICE_OBJECT pDeviceObject, IN PKEYBOARD_INPUT_DATA pInputDataStart, IN PKEYBOARD_INPUT_DATA pInputDataEnd, IN OUT PULONG pInputDataConsumed);
// 在鼠标设备栈上，由MouClasss驱动对象上生成的设备对象的鼠标回调函数，该回调函数的函数指针在下层的设备对象上存有一份（i8042prt或者mouhid驱动对象的设备）
typedef VOID (__stdcall *MouseClassServiceCallback)(IN PDEVICE_OBJECT pDeviceObject, IN PMOUSE_INPUT_DATA pInputDataStart, IN PMOUSE_INPUT_DATA pInputDataEnd, IN OUT PULONG pInputDataConsumed);
// 打开驱动对象，该函数没有文档化，但确实存在，需要声明导出
NTSTATUS ObReferenceObjectByName(IN PUNICODE_STRING pDriverObjectName, IN ULONG nAttributes, IN PACCESS_STATE pPassedAccessState OPTIONAL, IN ACCESS_MASK nDesiredAccess OPTIONAL, IN POBJECT_TYPE pObjectType, IN KPROCESSOR_MODE nAccessMode, IN OUT PVOID pParseContext OPTIONAL, OUT PVOID* pObject);
// 同样，没有公开，但确实存在，需要声明导出
extern POBJECT_TYPE* IoDriverObjectType;
// 获取键盘设备驱动
PDRIVER_OBJECT GetDriverObject(IN PUNICODE_STRING pDriverName);
// 是否在某个地址空间中
INT IsInAddress(PVOID pTarget, PVOID pStart, ULONG nSize);
// 搜索KeyboardClassServiceCallback函数地址
NTSTATUS SearchKeyboardClassServiceCallbackAddress(IN PDEVICE_OBJECT pControlDeviceObject);
// 搜索MouseClassServiceCallback函数地址
NTSTATUS SearchMouseClassServiceCallback(IN PDEVICE_OBJECT pControlDeviceObject);


// 控制设备类型
#define CTL_DEVICE_TYPE							FILE_DEVICE_UNKNOWN
// 键盘鼠标控制设备名字
#define KBD_MOU_CTL_DEVICE_OBJECT_NAME			L"\\Device\\GmKMClass"
// 键盘鼠标链接符号
#define KBD_MOU_CTL_DEVICE_SYMBOL_NAME			L"\\??\\GmKMClass"
// PS/2键盘端口层驱动名字
#define PS2_KBD_PORT_DRIVER_NAME				L"\\Driver\\i8042prt"
// PS/2鼠标端口层驱动名字
#define PS2_MOU_PORT_DRIVER_NAME				L"\\Driver\\i8042prt"
// USB键盘端口层驱动名字
#define USB_KBD_PORT_DRIVER_NAME				L"\\Driver\\kbdhid"
// USB鼠标端口层驱动名字
#define USB_MOU_PORT_DRIVER_NAME				L"\\Driver\\mouhid"
// 键盘设备类层驱动名字
#define KBD_CLASS_DRIVER_NAME					L"\\Driver\\kbdclass"
// 鼠标设备类层驱动名字
#define MOU_CLASS_DRIVER_NAME					L"\\Driver\\mouclass"
// 键盘控制功能号
#define KBD_CTL_CODE							(ULONG)CTL_CODE(CTL_DEVICE_TYPE, 0x911, METHOD_BUFFERED, FILE_ANY_ACCESS)
// 鼠标控制功能号
#define MOU_CTL_CODE							(ULONG)CTL_CODE(CTL_DEVICE_TYPE, 0x912, METHOD_BUFFERED, FILE_ANY_ACCESS)

// 控制设备扩展设备
typedef struct _ControlDeviceExtension
{
	PDEVICE_OBJECT					m_pKeyboradDeviceObject;				// 键盘设备
	KeyboardClassServiceCallback	m_pfnKeyboardClassServiceCallback;		// 键盘回调函数
	PDEVICE_OBJECT					m_pMouseDeviceObject;					// 鼠标设备
	MouseClassServiceCallback		m_pfnMouseClassServiceCallback;			// 鼠标回调函数
}ControlDeviceExtension;

#endif // __Common_H__
