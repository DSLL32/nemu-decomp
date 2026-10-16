#include "StdAfx.h"
#include "gs_dll.h"
#include "gs_exeDlg.h"
#include "NewCodeDlg.h"
#include "PropertiesDlg.h"
#include "Addsubcode.h"
#include "GS_interpret.h"
#include "RW_INI_files.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDM_ABOUTBOX 0x0010

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();
    enum { IDD = IDD_ABOUTBOX };
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CGs_exeDlg, CDialog)

CGs_exeDlg::CGs_exeDlg(CWnd* pParent)
    : CDialog(CGs_exeDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON146);
    pApp = NULL;
}

CGs_exeDlg::~CGs_exeDlg()
{
    if (pApp) pApp->m_pMainWnd = NULL;
}

void CGs_exeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CGs_exeDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_DESTROY()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_CLOSE()
    ON_LBN_SELCHANGE(IDC_CODE_NAMES, OnSelchangeList)
    ON_BN_CLICKED(IDC_NEW_CODE, OnNewCode)
    ON_BN_CLICKED(IDC_REMOVE_CODE, OnRemoveCode)
    ON_BN_CLICKED(IDC_PROPERTIES, OnProperties)
    ON_LBN_DBLCLK(IDC_CODE_NAMES, OnDblclkCodelist)
    ON_BN_CLICKED(IDC_ADD_VALUE, OnAddcode)
    ON_BN_CLICKED(IDC_DELETE_VALUE, OnDelcode)
    ON_LBN_SELCHANGE(IDC_CODE_VALUES, OnSelchangeCodesublist)
    ON_LBN_SETFOCUS(IDC_CODE_NAMES, OnSetfocusCodelist)
    ON_BN_CLICKED(IDC_ACTIVATE_CODE, OnAddActive)
    ON_BN_CLICKED(IDC_DELETE_ACTIVE_CODE, OnDelActive)
    ON_LBN_DBLCLK(IDC_CODE_VALUES, OnDblclkCodesublist)
END_MESSAGE_MAP()

BOOL CGs_exeDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);
    InitGsCodes();
    return TRUE;
}

void CGs_exeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else CDialog::OnSysCommand(nID, lParam);
}

void CGs_exeDlg::OnDestroy()
{
    WinHelp(0L, HELP_QUIT);
    CDialog::OnDestroy();
}

void CGs_exeDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    }
    else CDialog::OnPaint();
}

HCURSOR CGs_exeDlg::OnQueryDragIcon()
{
    return (HCURSOR)m_hIcon;
}

void CGs_exeDlg::OnClose()
{
    if (CanExit()) CDialog::OnClose();
}

void CGs_exeDlg::OnOK()
{
    UpdateGsCodesInINI();
    if (CanExit()) CDialog::OnOK();
}

void CGs_exeDlg::OnCancel()
{
    if (CanExit()) CDialog::OnCancel();
}

BOOL CGs_exeDlg::CanExit()
{
    if (pApp)
    {
        ShowWindow(SW_HIDE);
        return FALSE;
    }
    return TRUE;
}

void CGs_exeDlg::OnSelchangeList()
{
    int i = 0;
    POSITION pos;
    CStringList* list;
    CString str;
    while (CodeSubList().GetCount()) CodeSubList().DeleteString(0);
    if (!CodeList().GetCount()) return;
    list = &Codes;
    if (list)
    {
        CodeList().GetText(CodeList().GetCurSel(), str);
        pos = list->Find(str);
        str = list->GetNext(pos);
        str = list->GetNext(pos);
        int count = atoi(str);
        for (i = 0; i < count; i++)
            CodeSubList().AddString(list->GetNext(pos));
    }
}

CListBox& CGs_exeDlg::CodeList()
{
    return *(CListBox*)GetDlgItem(IDC_CODE_NAMES);
}

CListBox& CGs_exeDlg::CodeSubList()
{
    return *(CListBox*)GetDlgItem(IDC_CODE_VALUES);
}

void CGs_exeDlg::OnNewCode()
{
    NewCodeDlg dlg;
    int result = dlg.DoModal();
    if (result == IDOK)
    {
        if (CodeList().FindString(0, dlg.m_CodeDesc) != LB_ERR)
        {
            MessageBox("Code name already in the list.", "ERROR", MB_ICONSTOP);
            return;
        }
        CodeList().InsertString(CodeList().GetCurSel() + 1, dlg.m_CodeDesc);
        CodeList().SetFocus();
        CodeList().SetCurSel(CodeList().GetCurSel() + 1);
        Codes.AddTail(dlg.m_CodeDesc);
        Codes.AddTail("0");
        NumCodes++;
        OnSelchangeList();
    }
}

void CGs_exeDlg::OnRemoveCode()
{
    int count, i;
    char text[256];
    POSITION namepos, pos, nextpos;
    CStringList* list;
    CString str;
    sprintf(text, "Are you sure you want to delete the code permanently from the code list?");
    if (MessageBox(text, "WARNING", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
    {
        list = &Codes;
        CodeList().GetText(CodeList().GetCurSel(), str);
        pos = list->Find(str);
        list->GetNext(pos);
        str = list->GetAt(pos);
        count = atoi(str);
        list->GetPrev(pos);
        namepos = pos;
        list->GetNext(pos);
        nextpos = pos;
        list->GetNext(nextpos);
        list->RemoveAt(pos);
        list->RemoveAt(namepos);
        for (i = 0; i < count; i++)
        {
            pos = nextpos;
            list->GetNext(pos);
            list->RemoveAt(nextpos);
            nextpos = pos;
        }
        int selected = CodeList().GetCurSel();
        CodeList().DeleteString(selected);
        if (selected > 0) CodeList().SetCurSel(selected - 1);
        else CodeList().SetCurSel(0);
        NumCodes--;
        OnSelchangeList();
    }
}

void CGs_exeDlg::OnProperties()
{
    int i, j;
    POSITION pos;
    CStringList* list;
    CString str;
    char text[256];
    char allowed[] = "ABCDEFGabcdefg1234567890";
    char invalid[257];
    for (i = 0; i < 256; i++) invalid[i] = i;
    invalid[256] = 0;
    invalid[0] = 'W';
    for (i = 0; i < 256; i++)
        for (j = 0; j < (int)strlen(allowed); j++)
            if (invalid[i] == allowed[j]) invalid[i] = 'W';
    PropertiesDlg dlg;
    int selected = CodeSubList().GetCurSel();
    if (CodeSubList().GetCurSel() != LB_ERR)
    {
        CodeSubList().GetText(CodeSubList().GetCurSel(), text);
        strncpy(dlg.m_CodeAddr.GetBuffer(8), text, 8);
        strncpy(dlg.m_CodeVal.GetBuffer(4), text + 9, 4);
    }
    else
    {
        MessageBox("Must select a code to modify first", "ERROR", MB_ICONSTOP);
        return;
    }
    int result = dlg.DoModal();
    if (result == IDOK)
    {
        if (dlg.m_CodeAddr.IsEmpty() || dlg.m_CodeVal.IsEmpty() ||
            dlg.m_CodeAddr.GetLength() != 8 || dlg.m_CodeVal.GetLength() != 4 ||
            dlg.m_CodeAddr.GetAt(2) > '7' || dlg.m_CodeAddr.GetAt(2) < '0' ||
            dlg.m_CodeAddr.FindOneOf(invalid) != -1 ||
            dlg.m_CodeVal.FindOneOf(invalid) != -1)
        {
            if (MessageBox("Code does not appear to be valid.\nUse it anyway?",
                "WARNING", MB_YESNO | MB_ICONEXCLAMATION) == IDNO) return;
        }
        list = &Codes;
        CodeList().GetText(CodeList().GetCurSel(), str);
        pos = list->Find(str);
        CodeSubList().GetText(CodeSubList().GetCurSel(), str);
        pos = list->Find(str, pos);
        CodeSubList().GetText(CodeSubList().GetCurSel(), text);
        strncpy(text, dlg.m_CodeAddr.GetBuffer(8), 8);
        strncpy(text + 9, dlg.m_CodeVal.GetBuffer(4), 4);
        text[13] = 0;
        list->InsertAfter(pos, text);
        list->RemoveAt(pos);
        OnSelchangeList();
        CodeSubList().SetCurSel(selected);
    }
}

void CGs_exeDlg::OnDblclkCodelist()
{
}

void CGs_exeDlg::OnDelcode()
{
    POSITION original, pos;
    CStringList* list;
    CString str;
    char text[16];
    int count;
    if (CodeSubList().GetCurSel() == LB_ERR) return;
    CodeList().GetText(CodeList().GetCurSel(), str);
    list = &Codes;
    pos = list->Find(str);
    original = pos;
    list->GetNext(pos);
    str = list->GetAt(pos);
    count = atoi(str);
    sprintf(text, "%d", count - 1);
    list->InsertBefore(pos, text);
    list->RemoveAt(pos);
    list = &Codes;
    CodeList().GetText(CodeList().GetCurSel(), str);
    pos = list->Find(str);
    CodeSubList().GetText(CodeSubList().GetCurSel(), str);
    pos = list->Find(str, pos);
    list->RemoveAt(pos);
    int selected = CodeSubList().GetCurSel();
    CodeSubList().DeleteString(selected);
    if (selected) CodeSubList().SetCurSel(selected - 1);
}

void CGs_exeDlg::OnAddcode()
{
    int count, i, j;
    char text[16];
    char counttext[16];
    POSITION original, pos;
    CStringList* list;
    CString str;
    char allowed[] = "ABCDEFGabcdefg1234567890";
    char invalid[257];
    for (i = 0; i < 256; i++) invalid[i] = i;
    invalid[256] = 0;
    invalid[0] = 'W';
    for (i = 0; i < 256; i++)
        for (j = 0; j < (int)strlen(allowed); j++)
            if (invalid[i] == allowed[j]) invalid[i] = 'W';
    if (CodeList().GetCurSel() == LB_ERR) return;
    if (CodeSubList().GetCurSel() == LB_ERR) CodeSubList().SetCurSel(0);
    Addsubcode dlg;
    int result = dlg.DoModal();
    if (result == IDOK)
    {
        if (dlg.m_SubcodeAddr.IsEmpty() || dlg.m_SubcodeVal.IsEmpty() ||
            dlg.m_SubcodeAddr.GetLength() != 8 || dlg.m_SubcodeVal.GetLength() != 4 ||
            dlg.m_SubcodeAddr.GetAt(2) > '7' || dlg.m_SubcodeAddr.GetAt(2) < '0' ||
            dlg.m_SubcodeAddr.FindOneOf(invalid) != -1 ||
            dlg.m_SubcodeVal.FindOneOf(invalid) != -1)
        {
            if (MessageBox("Code does not appear to be valid.\nWould you still like to add it?",
                "WARNING", MB_YESNO | MB_ICONEXCLAMATION) == IDNO) return;
        }
        memset(text, 0, 16);
        strncat(text, dlg.m_SubcodeAddr.GetBuffer(8), 8);
        strncat(text, " ", 1);
        strncat(text + 9, dlg.m_SubcodeVal.GetBuffer(4), 4);
        CodeList().GetText(CodeList().GetCurSel(), str);
        list = &Codes;
        pos = list->Find(str);
        original = pos;
        list->GetNext(pos);
        str = list->GetAt(pos);
        count = atoi(str);
        sprintf(counttext, "%d", count + 1);
        list->InsertBefore(pos, counttext);
        list->RemoveAt(pos);
        if (count == 0)
        {
            pos = original;
            list->GetNext(pos);
            list->InsertAfter(pos, text);
        }
        else
        {
            list = &Codes;
            CodeList().GetText(CodeList().GetCurSel(), str);
            pos = list->Find(str);
            CodeSubList().GetText(CodeSubList().GetCurSel(), str);
            pos = list->Find(str, pos);
            list->InsertAfter(pos, text);
        }
        CodeSubList().InsertString(CodeSubList().GetCurSel() + 1, text);
        CodeSubList().SetCurSel(CodeSubList().GetCurSel() + 1);
    }
}

void CGs_exeDlg::OnSelchangeCodesublist()
{
}

void CGs_exeDlg::OnSetfocusCodelist()
{
}

void CGs_exeDlg::OnDblclkCodesublist()
{
    OnProperties();
}

void CGs_exeDlg::OnAddActive()
{
    POSITION pos;
    CStringList* list;
    CString str;
    CodeList().GetText(CodeList().GetCurSel(), str);
    list = &Codes;
    pos = list->Find(str);
    list->GetNext(pos);
    str = list->GetNext(pos);
    if (!atoi(str))
    {
        MessageBox(" Code name has no code values.\n You must add code values before adding to active list", "ERROR", MB_ICONSTOP);
        return;
    }
    CodeList().GetText(CodeList().GetCurSel(), str);
    if (ActiveList().FindString(0, str) != LB_ERR) return;
    ActiveList().InsertString(ActiveList().GetCurSel(), str);
    GSUpdateList();
}

CListBox& CGs_exeDlg::ActiveList()
{
    return *(CListBox*)GetDlgItem(IDC_ACTIVE_CODE_NAMES);
}

void CGs_exeDlg::OnDelActive()
{
    ActiveList().DeleteString(ActiveList().GetCurSel());
    GSUpdateList();
}

void CGs_exeDlg::InitGsCodes()
{
    int i, j;
    char key[256], text[256], section[32768];
    char* entry;
    int length, count;
    NemuIniSetDirectory();
    length = NemuGetProfileSection(GameName, section, 32767);
    for (i = 0; i < length; i++)
        if ((toupper(section[i]) == 'W' || toupper(section[i]) == 'B') && section[i + 1] == ',') break;
    if (i != length)
    {
        while (section + i > section && section[i - 1]) i--;
        entry = section + i;
        while (*entry)
        {
            for (i = 0; i < (int)strlen(entry); i++)
                if ((toupper(entry[i]) == 'W' || toupper(entry[i]) == 'B') && entry[i + 1] == ',') break;
            if (i != (int)strlen(entry)) LegacyCodes.AddTail(entry);
            entry += strlen(entry) + 1;
        }
    }
    NumCodes = NemuGetProfileInt(GameName, "NumCheats", 0);
    if (!NumCodes) return;
    for (i = 0; i < NumCodes; i++)
    {
        sprintf(key, "CheatName%d", i);
        NemuGetProfileString(GameName, key, key, text);
        Codes.AddTail(text);
        CodeList().AddString(text);
        sprintf(key, "CheatName%dCount", i);
        count = NemuGetProfileInt(GameName, key, 0);
        sprintf(key, "%d", count);
        Codes.AddTail(key);
        for (j = 0; j < count; j++)
        {
            sprintf(key, "CheatName%dCode%d", i, j);
            NemuGetProfileString(GameName, key, "deadf00d beef", text);
            Codes.AddTail(text);
        }
    }
    CodeList().SetCurSel(0);
    OnSelchangeList();
    UpdateActiveList();
}

void CGs_exeDlg::UpdateGsCodesInINI()
{
    int i, j, count;
    char key[256];
    char* text;
    CString str;
    POSITION pos;
    CStringList* list;
    list = &Codes;
    pos = list->GetHeadPosition();
    NumCodes = CodeList().GetCount();
    NemuWriteProfileInt(GameName, "NumCheats", NumCodes);
    for (i = 0; i < NumCodes; i++)
    {
        sprintf(key, "CheatName%d", i);
        str = list->GetNext(pos);
        text = str.GetBuffer(str.GetLength());
        NemuWriteProfileString(GameName, key, text);
        sprintf(key, "CheatName%dCount", i);
        count = atoi(list->GetNext(pos));
        NemuWriteProfileInt(GameName, key, count);
        for (j = 0; j < count; j++)
        {
            sprintf(key, "CheatName%dCode%d", i, j);
            str = list->GetNext(pos);
            text = str.GetBuffer(str.GetLength());
            NemuWriteProfileString(GameName, key, text);
        }
    }
}
