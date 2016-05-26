//////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014,My Engine All Rights Reserved.
//
// FileName: Common.h
// Summary: 
//
// Version: 1.0
// Author: Chen Xiao
// Data: 2016��5��20�� 18:24
//////////////////////////////////////////////////////////////////////////
#ifndef __Common_H__
#define __Common_H__
#include <ntddk.h>
#include <ntddkbd.h>
#include <ntddmou.h>

// �ڼ����豸ջ�ϣ���KbdClasss�������������ɵ��豸����ļ��̻ص��������ûص������ĺ���ָ�����²���豸�����ϴ���һ�ݣ�i8042prt����kbdhid����������豸��
typedef VOID (__stdcall *KeyboardClassServiceCallback)(IN PDEVICE_OBJECT pDeviceObject, IN PKEYBOARD_INPUT_DATA pInputDataStart, IN PKEYBOARD_INPUT_DATA pInputDataEnd, IN OUT PULONG pInputDataConsumed);
// ������豸ջ�ϣ���MouClasss�������������ɵ��豸��������ص��������ûص������ĺ���ָ�����²���豸�����ϴ���һ�ݣ�i8042prt����mouhid����������豸��
typedef VOID (__stdcall *MouseClassServiceCallback)(IN PDEVICE_OBJECT pDeviceObject, IN PMOUSE_INPUT_DATA pInputDataStart, IN PMOUSE_INPUT_DATA pInputDataEnd, IN OUT PULONG pInputDataConsumed);
// ���������󣬸ú���û���ĵ�������ȷʵ���ڣ���Ҫ��������
NTSTATUS ObReferenceObjectByName(IN PUNICODE_STRING pDriverObjectName, IN ULONG nAttributes, IN PACCESS_STATE pPassedAccessState OPTIONAL, IN ACCESS_MASK nDesiredAccess OPTIONAL, IN POBJECT_TYPE pObjectType, IN KPROCESSOR_MODE nAccessMode, IN OUT PVOID pParseContext OPTIONAL, OUT PVOID* pObject);
// ͬ����û�й�������ȷʵ���ڣ���Ҫ��������
extern POBJECT_TYPE* IoDriverObjectType;
// ��ȡ�����豸����
PDRIVER_OBJECT GetDriverObject(IN PUNICODE_STRING pDriverName);
// �Ƿ���ĳ����ַ�ռ���
INT IsInAddress(PVOID pTarget, PVOID pStart, ULONG nSize);
// ����KeyboardClassServiceCallback������ַ
NTSTATUS SearchKeyboardClassServiceCallbackAddress(IN PDEVICE_OBJECT pControlDeviceObject);
// ����MouseClassServiceCallback������ַ
NTSTATUS SearchMouseClassServiceCallback(IN PDEVICE_OBJECT pControlDeviceObject);


// �����豸����
#define CTL_DEVICE_TYPE							FILE_DEVICE_UNKNOWN
// �����������豸����
#define KBD_MOU_CTL_DEVICE_OBJECT_NAME			L"\\Device\\GmKMClass"
// ����������ӷ���
#define KBD_MOU_CTL_DEVICE_SYMBOL_NAME			L"\\??\\GmKMClass"
// PS/2���̶˿ڲ���������
#define PS2_KBD_PORT_DRIVER_NAME				L"\\Driver\\i8042prt"
// PS/2���˿ڲ���������
#define PS2_MOU_PORT_DRIVER_NAME				L"\\Driver\\i8042prt"
// USB���̶˿ڲ���������
#define USB_KBD_PORT_DRIVER_NAME				L"\\Driver\\kbdhid"
// USB���˿ڲ���������
#define USB_MOU_PORT_DRIVER_NAME				L"\\Driver\\mouhid"
// �����豸�����������
#define KBD_CLASS_DRIVER_NAME					L"\\Driver\\kbdclass"
// ����豸�����������
#define MOU_CLASS_DRIVER_NAME					L"\\Driver\\mouclass"
// ���̿��ƹ��ܺ�
#define KBD_CTL_CODE							(ULONG)CTL_CODE(CTL_DEVICE_TYPE, 0x911, METHOD_BUFFERED, FILE_ANY_ACCESS)
// �����ƹ��ܺ�
#define MOU_CTL_CODE							(ULONG)CTL_CODE(CTL_DEVICE_TYPE, 0x912, METHOD_BUFFERED, FILE_ANY_ACCESS)

// �����豸��չ�豸
typedef struct _ControlDeviceExtension
{
	PDEVICE_OBJECT					m_pKeyboradDeviceObject;				// �����豸
	KeyboardClassServiceCallback	m_pfnKeyboardClassServiceCallback;		// ���̻ص�����
	PDEVICE_OBJECT					m_pMouseDeviceObject;					// ����豸
	MouseClassServiceCallback		m_pfnMouseClassServiceCallback;			// ���ص�����
}ControlDeviceExtension;

#endif // __Common_H__
