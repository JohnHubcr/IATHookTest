
// InjectDllToolDlg.h : ͷ�ļ�
//

#pragma once


// CInjectDllToolDlg �Ի���
class CInjectDllToolDlg : public CDialogEx
{
// ����
public:
	CInjectDllToolDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_INJECTDLLTOOL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnInjectDll();
	afx_msg void OnBnClickedBtnUninstallDll();
private:
	CString m_uiExePath;
	CString m_uiDllPath;
};
