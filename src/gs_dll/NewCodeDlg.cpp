// NewCodeDlg.cpp : implementation of the code description dialog.

#include "stdafx.h"
#include "gs_dll.h"
#include "NewCodeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

NewCodeDlg::NewCodeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(NewCodeDlg::IDD, pParent)
{
	m_CodeDesc = _T("");
}

void NewCodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NEWCODE_NAME, m_CodeDesc);
	DDV_MaxChars(pDX, m_CodeDesc, 64);
}

BEGIN_MESSAGE_MAP(NewCodeDlg, CDialog)
	ON_EN_CHANGE(IDC_NEWCODE_NAME, OnChangeCodeDesc)
END_MESSAGE_MAP()

void NewCodeDlg::OnChangeCodeDesc()
{
}
