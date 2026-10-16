// PropertiesDlg.cpp : implementation of the subcode properties dialog.

#include "stdafx.h"
#include "gs_dll.h"
#include "PropertiesDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

PropertiesDlg::PropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(PropertiesDlg::IDD, pParent)
{
	m_CodeAddr = _T("");
	m_CodeVal = _T("");
}

void PropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PROPERTIES_ADDRESS, m_CodeAddr);
	DDV_MaxChars(pDX, m_CodeAddr, 8);
	DDX_Text(pDX, IDC_PROPERTIES_VALUE, m_CodeVal);
	DDV_MaxChars(pDX, m_CodeVal, 4);
}

BEGIN_MESSAGE_MAP(PropertiesDlg, CDialog)
	ON_EN_CHANGE(IDC_PROPERTIES_ADDRESS, OnChangeCodeAddr)
	ON_EN_CHANGE(IDC_PROPERTIES_VALUE, OnChangeCodeVal)
END_MESSAGE_MAP()

void PropertiesDlg::OnChangeCodeAddr()
{
}

void PropertiesDlg::OnChangeCodeVal()
{
}
