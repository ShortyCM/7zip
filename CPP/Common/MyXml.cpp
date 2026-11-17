// MyXml.cpp

#include "StdAfx.h"

#include "MyXml.h"
#include "StringToInt.h"

static bool IsValidChar(char c)
{
  return
    (c >= 'a' && c <= 'z') ||
    (c >= 'A' && c <= 'Z') ||
    (c >= '0' && c <= '9') ||
    c == '-' || c == '_' || c == '.' || c == ':';
}

static unsigned GetNameCharSize(const char *s, bool first)
{
  const Byte head = (Byte)*s;
  if (head < 0x80)
    return IsValidChar((char)head) ? 1 : 0;

  unsigned size;
  UInt32 value, minimum;
  if (head >= 0xc2 && head < 0xe0)
  {
    size = 2; value = head & 0x1f; minimum = 0x80;
  }
  else if (head >= 0xe0 && head < 0xf0)
  {
    size = 3; value = head & 0x0f; minimum = 0x800;
  }
  else if (head >= 0xf0 && head <= 0xf4)
  {
    size = 4; value = head & 7; minimum = 0x10000;
  }
  else
    return 0;
  for (unsigned i = 1; i < size; ++i)
  {
    const Byte c = (Byte)s[i];
    if (c < 0x80 || c >= 0xc0)
      return 0;
    value = (value << 6) | (c & 0x3f);
  }
  if (value < minimum)
    return 0;

  // XML 1.0 Fifth Edition: Unicode NameStartChar and NameChar ranges.
  if ((!first && (value == 0xb7 || (value >= 0x300 && value <= 0x36f)
          || (value >= 0x203f && value <= 0x2040)))
      || (value >= 0xc0 && value <= 0xd6)
      || (value >= 0xd8 && value <= 0xf6)
      || (value >= 0xf8 && value <= 0x2ff)
      || (value >= 0x370 && value <= 0x37d)
      || (value >= 0x37f && value <= 0x1fff)
      || (value >= 0x200c && value <= 0x200d)
      || (value >= 0x2070 && value <= 0x218f)
      || (value >= 0x2c00 && value <= 0x2fef)
      || (value >= 0x3001 && value <= 0xd7ff)
      || (value >= 0xf900 && value <= 0xfdcf)
      || (value >= 0xfdf0 && value <= 0xfffd)
      || (value >= 0x10000 && value <= 0xeffff))
    return size;
  return 0;
}

static bool IsSpaceChar(char c)
{
  return (c == ' ' || c == '\t' || c == 0x0D || c == 0x0A);
}

#define SKIP_SPACES(s) while (IsSpaceChar(*s)) s++;

static const char *SkipComment(const char *s)
{
  const char *end = strstr(s + 4, "--");
  if (!end || end[2] != '>')
    return NULL;
  return end + 3;
}

static const char *SkipProcessingInstruction(const char *s)
{
  s += 2;
  const char *beg = s;
  if ((*s >= '0' && *s <= '9') || *s == '-' || *s == '.')
    return NULL;
  for (;;)
  {
    const unsigned size = GetNameCharSize(s, s == beg);
    if (!size)
      break;
    s += size;
  }
  if (s == beg || ((size_t)(s - beg) == 3
      && MyCharLower_Ascii(beg[0]) == 'x'
      && MyCharLower_Ascii(beg[1]) == 'm'
      && MyCharLower_Ascii(beg[2]) == 'l'))
    return NULL;
  if (!IsSpaceChar(*s) && !IsString1PrefixedByString2(s, "?>"))
    return NULL;
  const char *end = strstr(s, "?>");
  return end ? end + 2 : NULL;
}

static const char *SkipMarkup(const char *s)
{
  if (IsString1PrefixedByString2(s, "<!--"))
    return SkipComment(s);
  if (IsString1PrefixedByString2(s, "<?"))
    return SkipProcessingInstruction(s);
  return s;
}

static const char *SkipSpacesAndMarkup(const char *s, bool preserveTextSpace = false)
{
  const char *start = s;
  for (;;)
  {
    SKIP_SPACES(s)
    const char *next = SkipMarkup(s);
    if (next == s)
      return preserveTextSpace && *s != 0
          && (*s != '<' || IsString1PrefixedByString2(s, "<![CDATA[")) ? start : s;
    s = next;
    if (!s)
      return NULL;
  }
}

int CXmlItem::FindProp(const char *propName) const throw()
{
  FOR_VECTOR (i, Props)
    if (Props[i].Name.IsEqualTo(propName))
      return (int)i;
  return -1;
}

AString CXmlItem::GetPropVal(const char *propName) const
{
  const int index = FindProp(propName);
  if (index >= 0)
    return Props[(unsigned)index].Value;
  return AString();
}

bool CXmlItem::IsTagged(const char *tag) const throw()
{
  return (IsTag && Name.IsEqualTo(tag));
}

int CXmlItem::FindSubTag(const char *tag) const throw()
{
  FOR_VECTOR (i, SubItems)
    if (SubItems[i].IsTagged(tag))
      return (int)i;
  return -1;
}

const CXmlItem *CXmlItem::FindSubTag_GetPtr(const char *tag) const throw()
{
  FOR_VECTOR (i, SubItems)
  {
    const CXmlItem *p = &SubItems[i];
    if (p->IsTagged(tag))
      return p;
  }
  return NULL;
}

AString CXmlItem::GetSubString(bool trimLeft) const
{
  if (SubItems.Size() == 1)
  {
    const CXmlItem &item = SubItems[0];
    if (!item.IsTag)
    {
      const char *s = item.Name;
      if (trimLeft)
      {
        SKIP_SPACES(s)
      }
      return AString(s);
    }
  }
  return AString();
}

const AString * CXmlItem::GetSubStringPtr() const throw()
{
  if (SubItems.Size() == 1)
  {
    const CXmlItem &item = SubItems[0];
    if (!item.IsTag)
      return &item.Name;
  }
  return NULL;
}

AString CXmlItem::GetSubStringForTag(const char *tag, bool trimLeft) const
{
  const CXmlItem *item = FindSubTag_GetPtr(tag);
  if (item)
    return item->GetSubString(trimLeft);
  return AString();
}

const char * CXmlItem::ParseItem(const char *s, int numAllowedLevels)
{
  Props.Clear();
  SubItems.Clear();
  s = SkipSpacesAndMarkup(s, true);
  if (!s)
    return NULL;

  const char *beg = s;
  bool hasText = false;
  for (;;)
  {
    while (*s != 0 && *s != '<')
      s++;
    if (*s == 0)
      return NULL;
    const size_t num = (size_t)(s - beg);
    if (num)
    {
      if (hasText)
      {
        AString part;
        part.SetFrom_Chars_SizeT(beg, num);
        Name += part;
      }
      else
        Name.SetFrom_Chars_SizeT(beg, num);
      hasText = true;
    }
    if (IsString1PrefixedByString2(s, "<![CDATA["))
    {
      const char *text = s + 9;
      const char *end = strstr(text, "]]>");
      if (!end)
        return NULL;
      if (!hasText)
        Name.Empty();
      // Text nodes store escaped XML; preserve the literal meaning of CDATA.
      for (; text != end; ++text)
      {
        const char c = *text;
        if (c == '&') Name += "&amp;";
        else if (c == '<') Name += "&lt;";
        else if (c == '>') Name += "&gt;";
        else Name += c;
      }
      hasText = true;
      s = end + 3;
      beg = s;
      continue;
    }
    const char *next = SkipMarkup(s);
    if (next == s)
      break;
    s = next;
    if (!s)
      return NULL;
    beg = s;
  }
  if (hasText)
  {
    IsTag = false;
    return s;
  }
  
  IsTag = true;

  s++;
  SKIP_SPACES(s)

  beg = s;
  for (;;)
  {
    const unsigned size = GetNameCharSize(s, s == beg);
    if (!size)
      break;
    s += size;
  }
  if (s == beg || *s == 0)
    return NULL;
  Name.SetFrom_Chars_SizeT(beg, (size_t)(s - beg));

  for (;;)
  {
    beg = s;
    SKIP_SPACES(s)
    if (*s == '/')
    {
      s++;
      // SKIP_SPACES(s)
      if (*s != '>')
        return NULL;
      return s + 1;
    }
    if (*s == '>')
    {
      s++;
      if (numAllowedLevels == 0)
        return NULL;
      for (;;)
      {
        s = SkipSpacesAndMarkup(s, true);
        if (!s)
          return NULL;
        if (s[0] == '<' && s[1] == '/')
          break;
        CXmlItem &item = SubItems.AddNew();
        s = item.ParseItem(s, numAllowedLevels - 1);
        if (!s)
          return NULL;
      }

      s += 2;
      const unsigned len = Name.Len();
      const char *name = Name.Ptr();
      for (unsigned i = 0; i < len; i++)
        if (*s++ != *name++)
          return NULL;
      // s += len;
      SKIP_SPACES(s)
      if (s[0] != '>')
        return NULL;
      return s + 1;
    }
    if (beg == s)
      return NULL;

    // ReadProperty
    CXmlProp &prop = Props.AddNew();

    beg = s;
    for (;;)
    {
      const unsigned size = GetNameCharSize(s, s == beg);
      if (!size)
        break;
      s += size;
    }
    if (s == beg)
      return NULL;
    prop.Name.SetFrom_Chars_SizeT(beg, (size_t)(s - beg));
    
    SKIP_SPACES(s)
    if (*s != '=')
      return NULL;
    s++;
    SKIP_SPACES(s)
    const char quote = *s;
    if (quote != '\"' && quote != '\'')
      return NULL;
    s++;
    
    beg = s;
    for (;;)
    {
      char c = *s;
      if (c == 0)
        return NULL;
      if (c == quote)
        break;
      s++;
    }
    prop.Value.SetFrom_Chars_SizeT(beg, (size_t)(s - beg));
    s++;
  }
}

static const char * SkipHeader(const char *s Z7_lifetimebound, const char *startString, const char *endString)
{
  SKIP_SPACES(s)
  if (IsString1PrefixedByString2(s, startString))
  {
    s = strstr(s, endString);
    if (!s)
      return NULL;
    s += strlen(endString);
  }
  return s;
}

static const char *SkipDocType(const char *s)
{
  SKIP_SPACES(s)
  if (!IsString1PrefixedByString2(s, "<!DOCTYPE"))
    return s;
  s += 9;
  if (!IsSpaceChar(*s))
    return NULL;
  char quote = 0;
  unsigned depth = 0;
  for (;;)
  {
    const char c = *s;
    if (c == 0)
      return NULL;
    if (quote)
    {
      if (c == quote)
        quote = 0;
    }
    else if (c == '\"' || c == '\'')
      quote = c;
    else if (IsString1PrefixedByString2(s, "<!--") || IsString1PrefixedByString2(s, "<?"))
    {
      s = SkipMarkup(s);
      if (!s)
        return NULL;
      continue;
    }
    else if (c == '[')
      depth++;
    else if (c == ']')
    {
      if (depth == 0)
        return NULL;
      depth--;
    }
    else if (c == '>' && depth == 0)
      return s + 1;
    s++;
  }
}

void CXmlItem::AppendTo(AString &s) const
{
  if (IsTag)
    s += '<';
  const char *name = Name;
  if (!IsTag && *name)
  {
    const char *end = name;
    SKIP_SPACES(end)
    if (*end == 0)
    {
      // Keep significant text whitespace distinct from element indentation.
      const char c = *name++;
      s += c == ' ' ? "&#32;" : c == '\t' ? "&#9;" : c == '\r' ? "&#13;" : "&#10;";
    }
  }
  s += name;
  if (IsTag)
  {
    FOR_VECTOR (i, Props)
    {
      const CXmlProp &prop = Props[i];
      s.Add_Space();
      s += prop.Name;
      s += '=';
      const char quote = prop.Value.Find('\"') < 0 ? '\"' : '\'';
      s += quote;
      s += prop.Value;
      s += quote;
    }
    s += '>';
  }
  FOR_VECTOR (i, SubItems)
  {
    const CXmlItem &item = SubItems[i];
    item.AppendTo(s);
  }
  if (IsTag)
  {
    s += '<';
    s += '/';
    s += Name;
    s += '>';
  }
}

bool CXml::Parse(const char *s)
{
  if (IsString1PrefixedByString2(s, "\xef\xbb\xbf"))
    s += 3;
  s = SkipHeader(s, "<?xml",    "?>"); if (!s) return false;
  s = SkipSpacesAndMarkup(s); if (!s) return false;
  s = SkipDocType(s); if (!s) return false;

  s = Root.ParseItem(s, 1000);
  if (!s || !Root.IsTag)
    return false;
  s = SkipSpacesAndMarkup(s);
  if (!s)
    return false;
  return *s == 0;
}

/*
void CXml::AppendTo(AString &s) const
{
  Root.AppendTo(s);
}
*/


void z7_xml_DecodeString(AString &temp)
{
  char * const beg = temp.GetBuf();
  char *dest = beg;
  const char *p = beg;
  for (;;)
  {
    char c = *p++;
    if (c == 0)
      break;
    if (c == '&')
    {
      if (p[0] == '#')
      {
        const char *digits = p + 1;
        const bool isHex = (*digits == 'x');
        if (isHex)
          digits++;
        const char *end;
        const UInt32 number = isHex ? ConvertHexStringToUInt32(digits, &end)
            : ConvertStringToUInt32(digits, &end);
        if (*end == ';' && number != 0 && number <= 0x10ffff
            && (number < 0xd800 || number >= 0xe000)
            && number != 0xfffe && number != 0xffff)
        {
          p = end + 1;
          if (number < 0x80)
            c = (char)number;
          else
          {
            if (number < 0x800)
              *dest++ = (char)(0xc0 | (number >> 6));
            else
            {
              if (number < 0x10000)
                *dest++ = (char)(0xe0 | (number >> 12));
              else
              {
                *dest++ = (char)(0xf0 | (number >> 18));
                *dest++ = (char)(0x80 | ((number >> 12) & 0x3f));
              }
              *dest++ = (char)(0x80 | ((number >> 6) & 0x3f));
            }
            c = (char)(0x80 | (number & 0x3f));
          }
        }
      }
      else if (
          p[0] == 'a' &&
          p[1] == 'm' &&
          p[2] == 'p' &&
          p[3] == ';')
      {
        p += 4;
      }
      else if (
          p[0] == 'l' &&
          p[1] == 't' &&
          p[2] == ';')
      {
        p += 3;
        c = '<';
      }
      else if (
          p[0] == 'g' &&
          p[1] == 't' &&
          p[2] == ';')
      {
        p += 3;
        c = '>';
      }
      else if (
          p[0] == 'a' &&
          p[1] == 'p' &&
          p[2] == 'o' &&
          p[3] == 's' &&
          p[4] == ';')
      {
        p += 5;
        c = '\'';
      }
      else if (
          p[0] == 'q' &&
          p[1] == 'u' &&
          p[2] == 'o' &&
          p[3] == 't' &&
          p[4] == ';')
      {
        p += 5;
        c = '\"';
      }
    }
    *dest++ = c;
  }
  temp.ReleaseBuf_SetEnd((unsigned)(dest - beg));
}
