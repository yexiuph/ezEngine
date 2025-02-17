#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Variant.h>

class ezStreamReader;
class ezStreamWriter;

/// \brief Flags for entries in ezBlackboard.
struct EZ_CORE_DLL ezBlackboardEntryFlags
{
  using StorageType = ezUInt16;

  enum Enum
  {
    None = 0,
    Save = EZ_BIT(0),          ///< Include the entry during serialization
    OnChangeEvent = EZ_BIT(1), ///< Broadcast the 'ValueChanged' event when this entry's value is modified

    UserFlag0 = EZ_BIT(7),
    UserFlag1 = EZ_BIT(8),
    UserFlag2 = EZ_BIT(9),
    UserFlag3 = EZ_BIT(10),
    UserFlag4 = EZ_BIT(11),
    UserFlag5 = EZ_BIT(12),
    UserFlag6 = EZ_BIT(13),
    UserFlag7 = EZ_BIT(14),

    Invalid = EZ_BIT(15),

    Default = None
  };

  struct Bits
  {
    StorageType Save : 1;
    StorageType OnChangeEvent : 1;
    StorageType Reserved : 5;
    StorageType UserFlag0 : 1;
    StorageType UserFlag1 : 1;
    StorageType UserFlag2 : 1;
    StorageType UserFlag3 : 1;
    StorageType UserFlag4 : 1;
    StorageType UserFlag5 : 1;
    StorageType UserFlag6 : 1;
    StorageType UserFlag7 : 1;
    StorageType Invalid : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezBlackboardEntryFlags);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezBlackboardEntryFlags);


/// \brief A blackboard is a key/value store that provides OnChange events to be informed when a value changes.
///
/// Blackboards are used to gather typically small pieces of data. Some systems write the data, other systems read it.
/// Through the blackboard, arbitrary systems can interact.
///
/// For example this is commonly used in game AI, where some system gathers interesting pieces of data about the environment,
/// and then NPCs might use that information to make decisions.
class EZ_CORE_DLL ezBlackboard : public ezRefCounted
{
private:
  ezBlackboard();

public:
  ~ezBlackboard();

  /// \brief Factory method to create a new blackboard.
  ///
  /// Since blackboards use shared ownership we need to make sure that blackboards are created in ezCore.dll.
  /// Some compilers (MSVC) create local v-tables which can become stale if a blackboard was registered as global but the DLL
  /// which created the blackboard is already unloaded.
  ///
  /// See https://groups.google.com/g/microsoft.public.vc.language/c/atSh_2VSc2w/m/EgJ3r_7OzVUJ?pli=1
  static ezSharedPtr<ezBlackboard> Create(ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());

  /// \brief Factory method to get access to a globally registered blackboard.
  ///
  /// If a blackboard with that name was already created globally before, its reference is returned.
  /// Otherwise it will be created and permanently registered under that name.
  /// Global blackboards cannot be removed. Although you can change their name via "SetName()",
  /// the name under which they are registered globally will not change.
  ///
  /// If at some point you want to "remove" a global blackboard, instead call UnregisterAllEntries() to
  /// clear all its values.
  static ezSharedPtr<ezBlackboard> GetOrCreateGlobal(const ezHashedString& sBlackboardName, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());

  /// \brief Finds a global blackboard with the given name.
  static ezSharedPtr<ezBlackboard> FindGlobal(const ezTempHashedString& sBlackboardName);

  /// \brief Changes the name of the blackboard.
  ///
  /// \note For global blackboards this has no effect under which name they are found. A global blackboard continues to
  /// be found by the name under which it was originally registered.
  void SetName(ezStringView sName);
  const char* GetName() const { return m_sName; }
  const ezHashedString& GetNameHashed() const { return m_sName; }

  struct Entry
  {
    ezVariant m_Value;
    ezBitflags<ezBlackboardEntryFlags> m_Flags;

    /// The change counter is increased every time the entry's value changes.
    /// Read this and compare it to a previous known value, to detect whether the value was changed since the last check.
    ezUInt32 m_uiChangeCounter = 0;
  };

  struct EntryEvent
  {
    ezHashedString m_sName;
    ezVariant m_OldValue;
    const Entry* m_pEntry;
  };

  /// \brief Registers an entry with a name, value and flags.
  ///
  /// If the entry already exists, it will add the entry flags that hadn't been set before, but NOT change the value.
  /// Thus you can use it to make sure that a value exists with a given start value, but keep it unchanged, if it already existed.
  void RegisterEntry(const ezHashedString& sName, const ezVariant& initialValue, ezBitflags<ezBlackboardEntryFlags> flags = ezBlackboardEntryFlags::None);

  /// \brief Removes the named entry. Does nothing, if no such entry exists.
  void UnregisterEntry(const ezHashedString& sName);

  ///  \brief Removes all entries.
  void UnregisterAllEntries();

  /// \brief Sets the value of the named entry.
  ///
  /// If the entry doesn't exist, EZ_FAILURE is returned.
  ///
  /// If the 'OnChangeEvent' flag is set for this entry, OnEntryEvent() will be broadcast.
  /// However, if the new value is no different to the old, no event will be broadcast, unless 'force' is set to true.
  ///
  /// Returns EZ_FAILURE, if the named entry hasn't been registered before.
  ezResult SetEntryValue(const ezTempHashedString& sName, const ezVariant& value, bool bForce = false);

  /// \brief Returns a pointer to the named entry, or nullptr if no such entry was registered.
  const Entry* GetEntry(const ezTempHashedString& sName) const;

  /// \brief Returns the flags of the named entry, or ezBlackboardEntryFlags::Invalid, if no such entry was registered.
  ezBitflags<ezBlackboardEntryFlags> GetEntryFlags(const ezTempHashedString& sName) const;

  /// \brief Returns the value of the named entry, or the fallback ezVariant, if no such entry was registered.
  ezVariant GetEntryValue(const ezTempHashedString& sName, const ezVariant& fallback = ezVariant()) const;

  /// \brief Grants read access to the entire map of entries.
  const ezHashTable<ezHashedString, Entry>& GetAllEntries() const { return m_Entries; }

  /// \brief Allows you to register to the OnEntryEvent. This is broadcast whenever an entry is modified that has the flag ezBlackboardEntryFlags::OnChangeEvent.
  const ezEvent<EntryEvent>& OnEntryEvent() const { return m_EntryEvents; }

  /// \brief This counter is increased every time an entry is added or removed (but not when it is modified).
  ///
  /// Comparing this value to a previous known value allows to quickly detect whether the set of entries has changed.
  ezUInt32 GetBlackboardChangeCounter() const { return m_uiBlackboardChangeCounter; }

  /// \brief This counter is increased every time any entry's value is modified.
  ///
  /// Comparing this value to a previous known value allows to quickly detect whether any entry has changed recently.
  ezUInt32 GetBlackboardEntryChangeCounter() const { return m_uiBlackboardEntryChangeCounter; }

  /// \brief Stores all entries that have the 'Save' flag in the stream.
  ezResult Serialize(ezStreamWriter& inout_stream) const;

  /// \brief Restores entries from the stream.
  ///
  /// If the blackboard already contains entries, the deserialized data is ADDED to the blackboard.
  /// If deserialized entries overlap with existing ones, the deserialized entries will overwrite the existing ones (both values and flags).
  ezResult Deserialize(ezStreamReader& inout_stream);

private:
  EZ_ALLOW_PRIVATE_PROPERTIES(ezBlackboard);

  static ezBlackboard* Reflection_GetOrCreateGlobal(ezStringView sName);
  static ezBlackboard* Reflection_FindGlobal(ezStringView sName);
  void Reflection_RegisterEntry(ezStringView sName, const ezVariant& initialValue, bool bSave, bool bOnChangeEvent);
  bool Reflection_SetEntryValue(ezStringView sName, const ezVariant& value);
  ezVariant Reflection_GetEntryValue(ezStringView sName, const ezVariant& fallback) const;

  ezHashedString m_sName;
  ezEvent<EntryEvent> m_EntryEvents;
  ezUInt32 m_uiBlackboardChangeCounter = 0;
  ezUInt32 m_uiBlackboardEntryChangeCounter = 0;
  ezHashTable<ezHashedString, Entry> m_Entries;

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, Blackboard);
  static ezMutex s_GlobalBlackboardsMutex;
  static ezHashTable<ezHashedString, ezSharedPtr<ezBlackboard>> s_GlobalBlackboards;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezBlackboard);

//////////////////////////////////////////////////////////////////////////

struct EZ_CORE_DLL ezBlackboardCondition
{
  ezHashedString m_sEntryName;
  double m_fComparisonValue = 0.0;
  ezEnum<ezComparisonOperator> m_Operator;

  bool IsConditionMet(const ezBlackboard& blackboard) const;

  ezResult Serialize(ezStreamWriter& inout_stream) const;
  ezResult Deserialize(ezStreamReader& inout_stream);

  const char* GetEntryName() const { return m_sEntryName; }
  void SetEntryName(const char* szName) { m_sEntryName.Assign(szName); }
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezBlackboardCondition);
