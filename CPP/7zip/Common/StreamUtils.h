// StreamUtils.h

#ifndef ZIP7_INC_STREAM_UTILS_H
#define ZIP7_INC_STREAM_UTILS_H

#include "../IStream.h"

inline HRESULT Stream_AddOffset(UInt64 base, Int64 offset, Int64 &position) throw()
{
  UInt64 pos;
  if (offset < 0)
  {
    const UInt64 distance = (UInt64)0 - (UInt64)offset;
    if (distance > base)
      return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
    pos = base - distance;
  }
  else
  {
    pos = base + (UInt64)offset;
    if (pos < base)
      return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  }
  if (pos >= ((UInt64)1 << 63))
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  position = (Int64)pos;
  return S_OK;
}

inline HRESULT InStream_SeekSet(IInStream *stream, UInt64 offset) throw()
  {  return stream->Seek((Int64)offset, STREAM_SEEK_SET, NULL); }
inline HRESULT InStream_GetPos(IInStream *stream, UInt64 &curPosRes) throw()
  {  return stream->Seek(0, STREAM_SEEK_CUR, &curPosRes); }
inline HRESULT InStream_GetSize_SeekToEnd(IInStream *stream, UInt64 &sizeRes) throw()
  { return stream->Seek(0, STREAM_SEEK_END, &sizeRes); }

HRESULT InStream_SeekToBegin(IInStream *stream) throw();
HRESULT InStream_AtBegin_GetSize(IInStream *stream, UInt64 &size) throw();
HRESULT InStream_GetPos_GetSize(IInStream *stream, UInt64 &curPosRes, UInt64 &sizeRes) throw();

inline HRESULT InStream_GetSize_SeekToBegin(IInStream *stream, UInt64 &sizeRes) throw()
{
  RINOK(InStream_SeekToBegin(stream))
  return InStream_AtBegin_GetSize(stream, sizeRes);
}


HRESULT ReadStream(ISequentialInStream *stream, void *data, size_t *size) throw();
HRESULT ReadStream_FALSE(ISequentialInStream *stream, void *data, size_t size) throw();
HRESULT ReadStream_FAIL(ISequentialInStream *stream, void *data, size_t size) throw();
HRESULT WriteStream(ISequentialOutStream *stream, const void *data, size_t size) throw();

#endif
