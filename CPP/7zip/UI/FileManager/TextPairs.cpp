// TextPairs.cpp

#include "StdAfx.h"

#include "TextPairs.h"

static const wchar_t kNewLineChar = '\n';
static const wchar_t kQuoteChar = '\"';

static const wchar_t kBOM = (wchar_t)0xFEFF;

static bool IsSeparatorChar(wchar_t c)
{
  return (c == ' ' || c == '\t');
}

static void RemoveCr(UString &s)
{
  s.RemoveChar(L'\x0D');
}

static UString GetIDString(const wchar_t *srcString, unsigned &finishPos, bool &lineEnd)
{
  UString result;
  bool quotes = false;
  lineEnd = false;
  for (finishPos = 0;;)
  {
    wchar_t c = srcString[finishPos];
    if (c == 0)
      break;
    finishPos++;
    bool isSeparatorChar = IsSeparatorChar(c);
    if (c == kNewLineChar)
    {
      lineEnd = true;
      break;
    }
    if ((isSeparatorChar && !quotes)
        || (c == kQuoteChar && quotes))
      break;
    else if (c == kQuoteChar)
      quotes = true;
    else
      result += c;
  }
  if (!quotes)
    result.Trim();
  RemoveCr(result);
  return result;
}

static UString GetValueString(const wchar_t *srcString, unsigned &finishPos)
{
  UString result;
  for (finishPos = 0;;)
  {
    wchar_t c = srcString[finishPos];
    if (c == 0)
      break;
    finishPos++;
    if (c == kNewLineChar)
      break;
    result += c;
  }
  RemoveCr(result);
  result.Trim();
  return result;
}

static bool GetTextPairs(const UString &srcString, CObjectVector<CTextPair> &pairs)
{
  pairs.Clear();
  unsigned pos = 0;
  
  if (srcString.Len() > 0)
  {
    if (srcString[0] == kBOM)
      pos++;
  }
  while (pos < srcString.Len())
  {
    unsigned finishPos;
    bool lineEnd;
    UString id = GetIDString((const wchar_t *)srcString + pos, finishPos, lineEnd);
    pos += finishPos;
    if (id.IsEmpty())
      continue;
    UString value;
    if (!lineEnd)
    {
      value = GetValueString((const wchar_t *)srcString + pos, finishPos);
      pos += finishPos;
    }
    if (!id.IsEmpty())
    {
      CTextPair pair;
      pair.ID = id;
      pair.Value = value;
      pairs.Add(pair);
    }
  }
  return true;
}

static int ComparePairIDs(const UString &s1, const UString &s2)
  { return MyStringCompareNoCase(s1, s2); }

static int ComparePairItems(const CTextPair &p1, const CTextPair &p2)
  { return ComparePairIDs(p1.ID, p2.ID); }

static int ComparePairItems(void *const *a1, void *const *a2, void * /* param */)
  { return ComparePairItems(**(const CTextPair *const *)a1, **(const CTextPair *const *)a2); }

void CPairsStorage::Sort() { Pairs.Sort(ComparePairItems, NULL); }

int CPairsStorage::FindID(const UString &id, unsigned &insertPos) const
{
  unsigned left = 0, right = Pairs.Size();
  while (left != right)
  {
    const unsigned mid = (left + right) / 2;
    const int compResult = ComparePairIDs(id, Pairs[mid].ID);
    if (compResult == 0)
    {
      insertPos = mid; // to disable GCC warning
      return (int)mid;
    }
    if (compResult < 0)
      right = mid;
    else
      left = mid + 1;
  }
  insertPos = left;
  return -1;
}

int CPairsStorage::FindID(const UString &id) const
{
  unsigned pos;
  return FindID(id, pos);
}

static void FindEqualPairRange(const CObjectVector<CTextPair> &pairs, unsigned pos,
    unsigned &begin, unsigned &end)
{
  const UString &id = pairs[pos].ID;
  begin = pos;
  end = pos + 1;
  while (begin != 0 && ComparePairIDs(id, pairs[begin - 1].ID) == 0)
    begin--;
  while (end < pairs.Size() && ComparePairIDs(id, pairs[end].ID) == 0)
    end++;
}

static void DeletePairRange(CObjectVector<CTextPair> &pairs, unsigned begin, unsigned end)
{
  if (begin == end)
    return;
  if (end == begin + 1)
  {
    pairs.Delete(begin);
    return;
  }
  // Compact the surviving tail once. MoveOneItem leaves null source slots.
  unsigned dest = begin;
  for (unsigned src = end; src < pairs.Size(); src++)
    pairs.MoveOneItem(dest++, src);
  pairs.DeleteFrom(dest);
}

void CPairsStorage::AddPair(const CTextPair &pair)
{
  unsigned insertPos;
  const int pos = FindID(pair.ID, insertPos);
  if (pos >= 0)
  {
    unsigned begin, end;
    FindEqualPairRange(Pairs, (unsigned)pos, begin, end);
    Pairs[begin] = pair;
    DeletePairRange(Pairs, begin + 1, end);
  }
  else
    Pairs.Insert(insertPos, pair);
}

void CPairsStorage::DeletePair(const UString &id)
{
  const int pos = FindID(id);
  if (pos >= 0)
  {
    unsigned begin, end;
    FindEqualPairRange(Pairs, (unsigned)pos, begin, end);
    DeletePairRange(Pairs, begin, end);
  }
}

bool CPairsStorage::GetValue(const UString &id, UString &value) const
{
  const int pos = FindID(id);
  value.Empty();
  if (pos < 0)
    return false;
  value = Pairs[pos].Value;
  return true;
}

UString CPairsStorage::GetValue(const UString &id) const
{
  const int pos = FindID(id);
  if (pos < 0)
    return UString();
  return Pairs[pos].Value;
}

bool CPairsStorage::ReadFromString(const UString &text)
{
  bool result = ::GetTextPairs(text, Pairs);
  if (result)
    Sort();
  else
    Pairs.Clear();
  return result;
}

void CPairsStorage::SaveToString(UString &text) const
{
  FOR_VECTOR (i, Pairs)
  {
    const CTextPair &pair = Pairs[i];
    bool multiWord = (pair.ID.Find(L' ') >= 0 || pair.ID.Find(L'\t') >= 0);
    if (multiWord)
      text.Add_Char('\"');
    text += pair.ID;
    if (multiWord)
      text.Add_Char('\"');
    text.Add_Space();
    text += pair.Value;
    text.Add_Char('\x0D');
    text.Add_LF();
  }
}
