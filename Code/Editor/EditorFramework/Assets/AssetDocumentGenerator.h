#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Status.h>

class ezDocument;

enum class ezAssetDocGeneratorPriority
{
  Undecided,
  LowPriority,
  DefaultPriority,
  HighPriority,
  ENUM_COUNT
};

class EZ_EDITORFRAMEWORK_DLL ezAssetDocumentGenerator : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetDocumentGenerator, ezReflectedClass);

public:
  ezAssetDocumentGenerator();
  ~ezAssetDocumentGenerator();

  struct Info
  {
    ezAssetDocumentGenerator* m_pGenerator = nullptr; ///< automatically set by ezAssetDocumentGenerator
    ezAssetDocGeneratorPriority m_Priority;           ///< has to be specified by generator
    ezString m_sOutputFileParentRelative;             ///< has to be specified by generator
    ezString m_sOutputFileAbsolute;                   ///< automatically generated from m_sOutputFileParentRelative
    ezString m_sName;                                 ///< has to be specified by generator, used to know which action to take by Generate()
    ezString m_sIcon;                                 ///< has to be specified by generator
  };

  struct ImportData
  {
    ezString m_sGroup;
    ezString m_sInputFileRelative;
    ezString m_sInputFileParentRelative;
    ezString m_sInputFileAbsolute;
    ezInt32 m_iSelectedOption = -1;
    ezString m_sImportMessage; // error text or "already exists"
    bool m_bDoNotImport = false;

    ezHybridArray<ezAssetDocumentGenerator::Info, 4> m_ImportOptions;
  };

  static void ImportAssets();
  static void ImportAssets(const ezHybridArray<ezString, 16>& filesToImport);
  static void ExecuteImport(ezDynamicArray<ezAssetDocumentGenerator::ImportData>& ref_allImports);

  virtual void GetImportModes(ezStringView sParentDirRelativePath, ezHybridArray<ezAssetDocumentGenerator::Info, 4>& out_modes) const = 0;
  virtual ezStatus Generate(ezStringView sDataDirRelativePath, const ezAssetDocumentGenerator::Info& mode, ezDocument*& out_pGeneratedDocument) = 0;
  virtual ezStringView GetDocumentExtension() const = 0;
  virtual ezStringView GetGeneratorGroup() const = 0;

  bool SupportsFileType(ezStringView sFile) const;
  void BuildFileDialogFilterString(ezStringBuilder& out_sFilter) const;
  void AppendFileFilterStrings(ezStringBuilder& out_sFilter, bool& ref_bSemicolon) const;

protected:
  void AddSupportedFileType(ezStringView sExtension);

private:
  static void CreateGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& out_Generators);
  static void DestroyGenerators(ezHybridArray<ezAssetDocumentGenerator*, 16>& generators);
  static ezResult DetermineInputAndOutputFiles(ImportData& data, Info& option);
  static void SortAndSelectBestImportOption(ezDynamicArray<ezAssetDocumentGenerator::ImportData>& allImports);
  static void CreateImportOptionList(const ezHybridArray<ezString, 16>& filesToImport, ezDynamicArray<ezAssetDocumentGenerator::ImportData>& allImports, const ezHybridArray<ezAssetDocumentGenerator*, 16>& generators);

  ezHybridArray<ezString, 16> m_SupportedFileTypes;
};
