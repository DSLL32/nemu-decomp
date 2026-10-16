// Addsubcode.h : declaration of the subcode entry dialog.

#if !defined(AFX_ADDSUBCODE_H__INCLUDED_)
#define AFX_ADDSUBCODE_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif

#include "resource.h"

class Addsubcode : public CDialog
{
public:
	Addsubcode(CWnd* pParent = NULL);

	enum { IDD = IDD_ADDSUBCODE };
	CString m_SubcodeAddr;
	CString m_SubcodeVal;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg void OnChangeSubcodeaddr();
	afx_msg void OnChangeSubcodeval();
	DECLARE_MESSAGE_MAP()
};

#endif
