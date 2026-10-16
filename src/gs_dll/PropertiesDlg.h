// PropertiesDlg.h : declaration of the subcode properties dialog.

#if !defined(AFX_PROPERTIESDLG_H__INCLUDED_)
#define AFX_PROPERTIESDLG_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

#include "resource.h"

class PropertiesDlg : public CDialog
{
public:
	PropertiesDlg(CWnd* pParent = NULL);

	enum { IDD = IDD_PROPERTIES };
	CString m_CodeAddr;
	CString m_CodeVal;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg void OnChangeCodeAddr();
	afx_msg void OnChangeCodeVal();
	DECLARE_MESSAGE_MAP()
};

#endif
