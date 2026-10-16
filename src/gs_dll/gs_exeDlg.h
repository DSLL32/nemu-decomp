#if !defined(AFX_GS_EXEDLG_H__INCLUDED_)
#define AFX_GS_EXEDLG_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

class CGs_exeDlg : public CDialog
{
    DECLARE_DYNAMIC(CGs_exeDlg)
public:
    CGs_exeDlg(CWnd* pParent = NULL);
    virtual ~CGs_exeDlg();
    enum { IDD = IDD_GS_EXE_DIALOG };
    int NumCodes;
    char* GameName;
    CWinApp* pApp;
protected:
    HICON m_hIcon;
    CStringList Codes;
    CStringList LegacyCodes;
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    BOOL CanExit();
    CListBox& CodeList();
    CListBox& CodeSubList();
    CListBox& ActiveList();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnDestroy();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnClose();
    afx_msg void OnSelchangeList();
    afx_msg void OnNewCode();
    afx_msg void OnRemoveCode();
    afx_msg void OnProperties();
    afx_msg void OnDblclkCodelist();
    afx_msg void OnDelcode();
    afx_msg void OnAddcode();
    afx_msg void OnSelchangeCodesublist();
    afx_msg void OnSetfocusCodelist();
    afx_msg void OnDblclkCodesublist();
    afx_msg void OnAddActive();
    afx_msg void OnDelActive();
    DECLARE_MESSAGE_MAP()
public:
    void InitGsCodes();
    void UpdateGsCodesInINI();
    void GSUpdateList();
    void UpdateActiveList();
};
#endif
