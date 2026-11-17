// EnumFormatEtc.cpp

#include "StdAfx.h"

#include "EnumFormatEtc.h"
#include "../../IDecl.h"
#include "MyCom2.h"
#include "../../../Common/ComTry.h"

#include <string.h>

class CEnumFormatEtc Z7_final:
  public IEnumFORMATETC,
  public CMyUnknownImp
{
  Z7_COM_UNKNOWN_IMP_1_MT(IEnumFORMATETC)
    
  STDMETHOD(Next)(ULONG celt, FORMATETC *rgelt, ULONG *pceltFetched) Z7_override;
  STDMETHOD(Skip)(ULONG celt) Z7_override;
  STDMETHOD(Reset)(void) Z7_override;
  STDMETHOD(Clone)(IEnumFORMATETC **ppEnumFormatEtc) Z7_override;

  ULONG m_NumFormats;
  FORMATETC *m_Formats;
  ULONG m_Index;
public:
  CEnumFormatEtc();
  HRESULT Init(const FORMATETC *pFormatEtc, ULONG numFormats);
  ~CEnumFormatEtc();
};

static HRESULT DeepCopyFormatEtc(FORMATETC *dest, const FORMATETC *src)
{
  *dest = *src;
  if (src->ptd)
  {
    dest->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(src->ptd->tdSize);
    if (!dest->ptd)
      return E_OUTOFMEMORY;
    memcpy(dest->ptd, src->ptd, src->ptd->tdSize);
  }
  return S_OK;
}

CEnumFormatEtc::CEnumFormatEtc()
{
  _m_RefCount = 1;
  m_Index = 0;
  m_NumFormats = 0;
  m_Formats = NULL;
}

HRESULT CEnumFormatEtc::Init(const FORMATETC *pFormatEtc, ULONG numFormats)
{
  if (numFormats == 0)
    return S_OK;
  m_Formats = new FORMATETC[numFormats];
  if (!m_Formats)
    return E_OUTOFMEMORY;
  while (m_NumFormats < numFormats)
  {
    const HRESULT result = DeepCopyFormatEtc(&m_Formats[m_NumFormats], &pFormatEtc[m_NumFormats]);
    if (result != S_OK)
      return result;
    m_NumFormats++;
  }
  return S_OK;
}

CEnumFormatEtc::~CEnumFormatEtc()
{
  if (m_Formats)
  {
    for (ULONG i = 0; i < m_NumFormats; i++)
      if (m_Formats[i].ptd)
        CoTaskMemFree(m_Formats[i].ptd);
    delete []m_Formats;
  }
}

Z7_COMWF_B CEnumFormatEtc::Next(ULONG celt, FORMATETC *pFormatEtc, ULONG *pceltFetched)
{
  ULONG copied  = 0;
  if (pceltFetched)
    *pceltFetched = 0;
  if (!pFormatEtc)
    return E_INVALIDARG;
  while (copied < m_NumFormats - m_Index && copied < celt)
  {
    const HRESULT result = DeepCopyFormatEtc(&pFormatEtc[copied], &m_Formats[m_Index + copied]);
    if (result != S_OK)
    {
      while (copied != 0)
      {
        FORMATETC &item = pFormatEtc[--copied];
        if (item.ptd)
          CoTaskMemFree(item.ptd);
        item.ptd = NULL;
      }
      return result;
    }
    copied++;
  }
  m_Index += copied;
  if (pceltFetched)
    *pceltFetched = copied;
  return (copied == celt) ? S_OK : S_FALSE;
}

Z7_COMWF_B CEnumFormatEtc::Skip(ULONG celt)
{
  if (celt > m_NumFormats - m_Index)
  {
    m_Index = m_NumFormats;
    return S_FALSE;
  }
  m_Index += celt;
  return S_OK;
}

Z7_COMWF_B CEnumFormatEtc::Reset(void)
{
  m_Index = 0;
  return S_OK;
}

Z7_COMWF_B CEnumFormatEtc::Clone(IEnumFORMATETC ** ppEnumFormatEtc)
{
  HRESULT hResult = CreateEnumFormatEtc(m_NumFormats, m_Formats, ppEnumFormatEtc);
  if (hResult == S_OK)
    ((CEnumFormatEtc *)*ppEnumFormatEtc)->m_Index = m_Index;
  return hResult;
}

// replacement for SHCreateStdEnumFmtEtc
HRESULT CreateEnumFormatEtc(UINT numFormats, const FORMATETC *formats, IEnumFORMATETC **enumFormat)
{
  if (!enumFormat)
    return E_INVALIDARG;
  *enumFormat = NULL;
  if (numFormats != 0 && !formats)
    return E_INVALIDARG;
  COM_TRY_BEGIN
  CEnumFormatEtc *spec = new CEnumFormatEtc;
  if (!spec)
    return E_OUTOFMEMORY;
  CMyComPtr<IEnumFORMATETC> owner;
  owner.Attach(spec);
  const HRESULT result = spec->Init(formats, numFormats);
  if (result != S_OK)
    return result;
  *enumFormat = owner.Detach();
  return S_OK;
  COM_TRY_END
}
