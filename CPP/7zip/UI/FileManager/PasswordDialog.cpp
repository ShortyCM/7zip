// PasswordDialog.cpp

#include "StdAfx.h"

#include "PasswordDialog.h"

#include "../../../Windows/ErrorMsg.h"
#include "../Explorer/MyMessages.h"

#ifdef Z7_LANG
#include "LangUtils.h"
#endif

#ifdef Z7_LANG
static const UInt32 kLangIDs[] =
{
  IDT_PASSWORD_ENTER,
  IDX_PASSWORD_SHOW
};
#endif

bool CPasswordDialog::ReadControls()
{
  UString password;
  if (!_passwordEdit.GetText(password))
  {
    const DWORD error = ::GetLastError();
    CheckButton(IDX_PASSWORD_SHOW, ShowPassword);
    ShowErrorMessage(*this, NWindows::NError::MyFormatMessage(error));
    return false;
  }
  Password = password;
  ShowPassword = IsButtonCheckedBool(IDX_PASSWORD_SHOW);
  return true;
}

void CPasswordDialog::SetTextSpec()
{
  _passwordEdit.SetPasswordChar(ShowPassword ? 0: TEXT('*'));
}

bool CPasswordDialog::OnInit()
{
  #ifdef Z7_LANG
  LangSetWindowText(*this, IDD_PASSWORD);
  LangSetDlgItems(*this, kLangIDs, Z7_ARRAY_SIZE(kLangIDs));
  #endif
  _passwordEdit.Attach(GetItem(IDE_PASSWORD_PASSWORD));
  CheckButton(IDX_PASSWORD_SHOW, ShowPassword);
  SetTextSpec();
  _passwordEdit.SetText(Password);
  return CModalDialog::OnInit();
}

bool CPasswordDialog::OnButtonClicked(unsigned buttonID, HWND buttonHWND)
{
  if (buttonID == IDX_PASSWORD_SHOW)
  {
    if (ReadControls())
      SetTextSpec();
    return true;
  }
  return CDialog::OnButtonClicked(buttonID, buttonHWND);
}

void CPasswordDialog::OnOK()
{
  if (!ReadControls())
    return;
  CModalDialog::OnOK();
}
