// OffsetStream.cpp

#include "StdAfx.h"

#include "OffsetStream.h"

HRESULT COffsetOutStream::Init(IOutStream *stream, UInt64 offset)
{
  _offset = offset;
  _virtPos = 0;
  _stream = stream;
  return _stream->Seek((Int64)offset, STREAM_SEEK_SET, NULL);
}

Z7_COM7F_IMF(COffsetOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize))
{
  if (processedSize)
    *processedSize = 0;
  if (size == 0)
    return S_OK;
  RINOK(_stream->Seek((Int64)(_offset + _virtPos), STREAM_SEEK_SET, NULL))
  UInt32 cur = 0;
  const HRESULT res = _stream->Write(data, size, &cur);
  _virtPos += cur;
  if (processedSize)
    *processedSize = cur;
  return res;
}

Z7_COM7F_IMF(COffsetOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition))
{
  UInt64 virtPos;
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET:
      if (offset < 0)
        return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
      virtPos = (UInt64)offset;
      break;
    case STREAM_SEEK_CUR:
      if (offset < 0)
      {
        const UInt64 distance = (UInt64)0 - (UInt64)offset;
        if (distance > _virtPos)
          return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
        virtPos = _virtPos - distance;
      }
      else
      {
        virtPos = _virtPos + (UInt64)offset;
        if (virtPos < _virtPos)
          return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
      }
      break;
    case STREAM_SEEK_END:
    {
      UInt64 pos = 0;
      RINOK(_stream->Seek(offset, STREAM_SEEK_END, &pos))
      if (pos < _offset)
        return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
      _virtPos = pos - _offset;
      if (newPosition)
        *newPosition = _virtPos;
      return S_OK;
    }
    default: return STG_E_INVALIDFUNCTION;
  }
  const UInt64 pos = _offset + virtPos;
  if (pos < _offset)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  RINOK(_stream->Seek((Int64)pos, STREAM_SEEK_SET, NULL))
  _virtPos = virtPos;
  if (newPosition)
    *newPosition = _virtPos;
  return S_OK;
}

Z7_COM7F_IMF(COffsetOutStream::SetSize(UInt64 newSize))
{
  const UInt64 size = _offset + newSize;
  if (size < newSize)
    return E_INVALIDARG;
  return _stream->SetSize(size);
}
