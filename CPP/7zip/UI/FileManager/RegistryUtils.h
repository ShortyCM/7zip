// RegistryUtils.h

#ifndef ZIP7_INC_REGISTRY_UTILS_H
#define ZIP7_INC_REGISTRY_UTILS_H

#include "../../../Common/MyTypes.h"
#include "../../../Common/MyString.h"
#include "../../../Common/MyWindows.h"

LONG SaveRegLang(const UString &path);
void ReadRegLang(UString &path);

LONG SaveRegEditor(bool useEditor, const UString &path);
void ReadRegEditor(bool useEditor, UString &path);

LONG SaveRegDiff(const UString &path);
void ReadRegDiff(UString &path);

void ReadReg_VerCtrlPath(UString &path);

struct CFmSettings
{
  bool ShowDots;
  bool ShowRealFileIcons;
  bool FullRow;
  bool ShowGrid;
  bool SingleClick;
  bool AlternativeSelection;
  // bool Underline;

  bool ShowSystemMenu;

  LONG Save() const;
  void Load();
};

// void SaveLockMemoryAdd(bool enable);
// bool ReadLockMemoryAdd();

bool ReadLockMemoryEnable();
LONG SaveLockMemoryEnable(bool enable);

void SaveFlatView(UInt32 panelIndex, bool enable);
bool ReadFlatView(UInt32 panelIndex);

/*
void Save_ShowDeleted(bool enable);
bool Read_ShowDeleted();
*/

#endif
