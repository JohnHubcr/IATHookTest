#include "stdafx.h"
#include <Windows.h>

DWORD g_funcAddrOrigninal = NULL; // CreateProcessW�����ĵ�ַ
DWORD g_funcIATfuncAddr = NULL; // �����ַ��ĵ�ַ�����Ǵ�ź�����ַ�ĵ�ַ������ж��IAT Hook

typedef BOOL (*CreateProcessWFunc)(
	LPCWSTR               lpApplicationName,
	LPWSTR                lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL                  bInheritHandles,
	DWORD                 dwCreationFlags,
	LPVOID                lpEnvironment,
	LPCWSTR               lpCurrentDirectory,
	LPSTARTUPINFOW        lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
	);



BOOL MyCreateProcessW(
	LPCWSTR               lpApplicationName,
	LPWSTR                lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL                  bInheritHandles,
	DWORD                 dwCreationFlags,
	LPVOID                lpEnvironment,
	LPCWSTR               lpCurrentDirectory,
	LPSTARTUPINFOW        lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
	)
{
	OutputDebugString(_T("MyCreateProcessW enter-----"));


	OutputDebugString(lpApplicationName);
	OutputDebugString(lpCommandLine);
	CreateProcessWFunc func = (CreateProcessWFunc)g_funcAddrOrigninal;
	BOOL ret = func(lpApplicationName,lpCommandLine,
		lpProcessAttributes,lpThreadAttributes,
		bInheritHandles,dwCreationFlags,
		lpEnvironment,lpCurrentDirectory,
		lpStartupInfo,lpProcessInformation);

	OutputDebugString(_T("MyCreateProcessW exit-----"));
	return ret;
}



 

void IATHOOKCreateProcessW()
{
	OutputDebugString(_T("IATHOOKCreateProcessW, enter "));
	//HMODULE hModuleExe = GetModuleHandle(NULL);
	// ��win7�µ�explorer.exe�� ʹ�õ���SHELL32.dll�е�kernel.dll!!!����������ʵ��ַӦ����SHELL32.dll
	HMODULE hModuleExe = GetModuleHandle(_T("SHELL32.dll"));

	// ��ȡCreateProcessW������ַ
	HMODULE hModuleKernel = GetModuleHandle(_T("KERNEL32.dll")); 
	if(hModuleKernel == NULL)
	{
		OutputDebugString(_T("IATHOOKCreateProcessW,LoadLibrary kernel32.dll failed !!!"));
		return;
	}
	CreateProcessWFunc CreateProcessWAddress = (CreateProcessWFunc)GetProcAddress(hModuleKernel,"CreateProcessW");
	if(CreateProcessWAddress == NULL)
	{
		OutputDebugString(_T("IATHOOKCreateProcessW,GetProcAddress CreateProcessW failed !!!"));
		return;
	}
	g_funcAddrOrigninal = (DWORD)CreateProcessWAddress;

	// ��ȡPE�ṹ
	PIMAGE_DOS_HEADER pDosHead = (PIMAGE_DOS_HEADER)hModuleExe;
	PIMAGE_NT_HEADERS pNtHead = (PIMAGE_NT_HEADERS)((DWORD)hModuleExe + pDosHead->e_lfanew);

	// ����ӳ���ַ�͵�����RVA
	ULONGLONG dwImageBase = pNtHead->OptionalHeader.ImageBase;
	ULONGLONG dwImpDicRva = pNtHead->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

	// ������VA��������һ���Ӧһ��DLLģ��
	PIMAGE_IMPORT_DESCRIPTOR pImageDes= (PIMAGE_IMPORT_DESCRIPTOR)(dwImageBase + dwImpDicRva);
	PIMAGE_IMPORT_DESCRIPTOR pImageTemp = pImageDes;

	// �ڵ�����в���Ҫhook��ģ���Ƿ����
	bool bFind = false;
	while(pImageTemp->Name) // ���һ��ṹ��Ϊȫ0
	{
		char* pName = (char*)(dwImageBase + pImageTemp->Name); // name��ַ
		CString cstrName = pName;
		if(cstrName.CompareNoCase(_T("kernel32.dll")) == 0)
		{
			OutputDebugString(_T("IATHOOKCreateProcessW,find kernel32.dll"));
			bFind = true;
			break;
		}
		pImageTemp++;
	}
	//return;
	bool bFindFnc = false;
	// �ҵ�ҪHOOK��DLLģ��
	if(bFind)
	{
		// �����ַ��һ���Ӧһ�����������б��� ���ҵ�Ҫhook�ĺ���
		PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)(dwImageBase + pImageTemp->FirstThunk);
		while(pThunk->u1.Function) // ���һ��ṹ��Ϊȫ0
		{
			DWORD* pFuncAddr = (DWORD*)&(pThunk->u1.Function); // �����ַ�ϴ�ŵ��ǡ������ĵ�ַ��
			
			// ȡ�������ĵ�ַ �� ֮ǰ�ڳ������ҵ��ĺ�����ַ���Ƚϣ����һ�����ҵ��˸ĺ����ĵ����ַ���ˣ�
			if(*pFuncAddr == g_funcAddrOrigninal)
			{
				bFindFnc = true;
				DWORD dwMyHookAddr = (DWORD)MyCreateProcessW;
				g_funcIATfuncAddr = (DWORD)pFuncAddr;

				OutputDebugString(_T("IATHOOKCreateProcessW,CreateProcessW find"));
				BOOL bRet = WriteProcessMemory(GetCurrentProcess(),pFuncAddr,&dwMyHookAddr,sizeof(DWORD),NULL);
				if(bRet)
				{
					OutputDebugString(_T("IATHOOKCreateProcessW,WriteProcessMemory suc"));
				}
				else
				{
					OutputDebugString(_T("IATHOOKCreateProcessW,WriteProcessMemory fail !!!"));
				}

				break;
			}
			pThunk++;
		}
	}

	if(bFindFnc == false)
	{
		OutputDebugString(_T("IATHOOKCreateProcessW, not find CreateProcessW������"));
	}

}

void UNIATHOOKCreateProcessW()
{
	OutputDebugString(_T("UNIATHOOKCreateProcessW, enter "));
	if(g_funcIATfuncAddr)
	{
		if(g_funcAddrOrigninal)
		{
			OutputDebugString(_T("UNIATHOOKCreateProcessW,CreateProcessW find"));
			BOOL bRet = WriteProcessMemory(GetCurrentProcess(),(LPVOID)g_funcIATfuncAddr,&g_funcAddrOrigninal,sizeof(DWORD),NULL);
			if(bRet)
			{
				OutputDebugString(_T("UNIATHOOKCreateProcessW,WriteProcessMemory suc"));
			}
			else
			{
				OutputDebugString(_T("UNIATHOOKCreateProcessW,WriteProcessMemory fail !!!"));
			}
		}
	}
}

BOOL WINAPI DllMain (
	HANDLE hInst,
	ULONG ul_reason_for_call,
	LPVOID lpReserved) {

	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH: 
		{
			IATHOOKCreateProcessW();
		}
		break;
	case DLL_PROCESS_DETACH: 
		{
			UNIATHOOKCreateProcessW();
		}
		break;
								
	case DLL_THREAD_ATTACH: 
		{
		}
		break;
								
	case DLL_THREAD_DETACH: 
		{
		}
		break;
	}

	return TRUE;
}