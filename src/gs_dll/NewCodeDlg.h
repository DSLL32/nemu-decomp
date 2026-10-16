// NewCodeDlg.h : declaration of the code description dialog.

#if !defined(AFX_NEWCODEDLG_H__INCLUDED_)
#define AFX_NEWCODEDLG_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

#include "resource.h"

class NewCodeDlg : public CDialog
{
public:
	NewCodeDlg(CWnd* pParent = NULL);

	enum { IDD = IDD_NEWCODE };
	CString m_CodeDesc;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg void OnChangeCodeDesc();
	DECLARE_MESSAGE_MAP()
};

#endif
