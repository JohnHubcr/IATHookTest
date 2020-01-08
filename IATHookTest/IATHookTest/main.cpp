#include "stdafx.h"
#include <Windows.h>

DWORD g_funcAddrOrigninal = NULL; // CreateProcessW�����ĵ�ַ
DWORD g_funcIATfuncAddr = NULL; // �����ַ��ĵ�ַ�����Ǵ�ź�����ַ�ĵ�ַ������ж��IAT Hook

typedef BOOL (WINAPI *CreateProcessWFunc)(
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



BOOL WINAPI MyCreateProcessW(
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
	BOOL ret = FALSE;
	CString appName = lpApplicationName;
	CString strMsg;
	strMsg.Format(_T("�Ƿ�򿪳���:%s "),appName);
	if(IDYES == MessageBox(NULL,strMsg,_T("��ѡ��"),MB_YESNO))
	{
		ret = func(lpApplicationName,lpCommandLine,
			lpProcessAttributes,lpThreadAttributes,
			bInheritHandles,dwCreationFlags,
			lpEnvironment,lpCurrentDirectory,
			lpStartupInfo,lpProcessInformation);
	}
	else
	{
		ret = TRUE;
	}

	OutputDebugString(_T("MyCreateProcessW exit-----"));
	return ret;
}



 

void IATHOOKCreateProcessW()
{
	OutputDebugString(_T("IATHOOKCreateProcessW, enter "));
	// ��ȡCreateProcessW������ַ���ú�����Kernel32.dll�����ĺ���
	HMODULE hModuleKernel = GetModuleHandle(_T("kernel32.dll")); 
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
	//CString addr;
	//addr.Format(_T("kernel->CreateProcessWAddress = %x"),g_funcAddrOrigninal);
	//OutputDebugString(addr);

	//HMODULE hModuleExe = GetModuleHandle(_T("SHELL32.dll"));
	// OD�Ͽ�����win7�µ�explorer.exe�� ʹ�õ���SHELL32.dll�е�kernel.dll!!!����������ʼ��ַӦ����SHELL32.dll
	// ��ʼ��ַ��SHELL32.dllģ�������ĵ�������ҵ���Ӧ��kernel32.dll��̬�⣬���Ǵ���kernel32.dll��IAT�Ҳ���CreateProcessW :)
	// �走�����ߵ�Ȼû�з��������߼����������������˸� DEPENDS.EXE ������ kernel32.dll��ȷʵ�ҵ���CreateProcessW��kernel32.dll��API-MS-WIN-CORE-PROCESSTHREADS-L1-1-0.DLLģ��ĵ���������
	// �����������ʼ��ַӦ��Ϊkernel32.dll��Ȼ������������е�API-MS-WIN-CORE-PROCESSTHREADS-L1-1-0.DLLģ�飬
	// Ȼ���API-MS-WIN-CORE-PROCESSTHREADS-L1-1-0.DLL������IAT�е�CreateProcessW��Ȼ�����hook
	//HMODULE hModuleExe = GetModuleHandle(_T("kernel32.dll"));
	// ���ǣ���kernel32.dll�ĵ�������ҵ���API-MS-WIN-CORE-PROCESSTHREADS-L1-1-0.DLL��IAT���ҵ��ĺ������ڴ��еĺ�����ַ��Ȼ�Բ��Ϻţ�
	// ���Ա�����ȥ����shell32.dll��ͨ��DEPENDS.exe���߷���shell32.dll��PE�ṹҲ����API-MS-WIN-CORE-PROCESSTHREADS-L1-1-0.DLL��
	// Ȼ���ٿ�shell32.dll�����ӵ�API-MS-WIN-CORE-PROCESSTHREADS-L1-1-0.DLL��Щ�Ǹ���ݷ�ʽ Ӧ�������ӵ�shell32.dll��API-MS-WIN-CORE-PROCESSTHREADS-L1-1-0.DLL
	// �����������ʼ��ַ����SHELL32.dllģ�飬ͬ����ȥ��API-MS-WIN-CORE-PROCESSTHREADS-L1-1-0.DLLȻ�������е�IAT
	HMODULE hModuleExe = GetModuleHandle(_T("shell32.dll"));


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
		if(cstrName.CompareNoCase(_T("API-MS-WIN-CORE-PROCESSTHREADS-L1-1-0.DLL")) == 0)
		{
			OutputDebugString(_T("IATHOOKCreateProcessW,find API-MS-WIN-CORE-PROCESSTHREADS-L1-1-0.DLL"));
			bFind = true;
			break;
		}
		pImageTemp++;
	}

	bool bFindFnc = false;
	// �Ѿ��ҵ�ҪHOOK��DLLģ��
	if(bFind)
	{
		// �����ַ��һ���Ӧһ�����������б��� ���ҵ�Ҫhook�ĺ���
		PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)(dwImageBase + pImageTemp->FirstThunk);
		while(pThunk->u1.Function) // ���һ��ṹ��Ϊȫ0
		{
			DWORD* pFuncAddr = (DWORD*)&(pThunk->u1.Function); // �����ַ���ڴ��ŵ��ǡ������ĵ�ַ��
			// ȡ�������ĵ�ַ �� ֮ǰ�ڳ������ҵ��ĺ�����ַ���Ƚϣ����һ�����ҵ��˸ú����ĵ����ַ���ˣ�
			//CString addr;
			//addr.Format(_T("IAT->funcAddr = %x"),*pFuncAddr);
			//OutputDebugString(addr);
			if(*pFuncAddr == g_funcAddrOrigninal)
			{
				bFindFnc = true;
				DWORD dwMyHookAddr = (DWORD)MyCreateProcessW;
				g_funcIATfuncAddr = (DWORD)pFuncAddr; // ����ź�����ַ���ڴ��ַ���棬�Ա����ж��hook

				OutputDebugString(_T("IATHOOKCreateProcessW, CreateProcessW was found"));
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
			OutputDebugString(_T("UNIATHOOKCreateProcessW,CreateProcessW was found"));
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