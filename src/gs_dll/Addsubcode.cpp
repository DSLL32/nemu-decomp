// Addsubcode.cpp : implementation of the subcode entry dialog.

#include "stdafx.h"
#include "gs_dll.h"
#include "Addsubcode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Addsubcode::Addsubcode(CWnd* pParent /*=NULL*/)
	: CDialog(Addsubcode::IDD, pParent)
{
	m_SubcodeAddr = _T("");
	m_SubcodeVal = _T("");
}

void Addsubcode::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_ADDSUBCODE_ADDRESS, m_SubcodeAddr);
	DDV_MaxChars(pDX, m_SubcodeAddr, 8);
	DDX_Text(pDX, IDC_ADDSUBCODE_VALUE, m_SubcodeVal);
	DDV_MaxChars(pDX, m_SubcodeVal, 4);
}

BEGIN_MESSAGE_MAP(Addsubcode, CDialog)
	ON_EN_CHANGE(IDC_ADDSUBCODE_ADDRESS, OnChangeSubcodeaddr)
	ON_EN_CHANGE(IDC_ADDSUBCODE_VALUE, OnChangeSubcodeval)
END_MESSAGE_MAP()

void Addsubcode::OnChangeSubcodeaddr()
{
}

void Addsubcode::OnChangeSubcodeval()
{
}
