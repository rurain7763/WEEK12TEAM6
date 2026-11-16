#pragma once

#include "Asset/Asset.h"
#include "Asset/AssetManager.h"
#include <concepts>
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/UObjectHash.h"
#include "Containers/Array.h"
#include "Core/EngineLog.h"
#include "Serialization/StructuredArchive.h"
#include "Serialization/StructuredArchiveSlots.h"

namespace StructuredArchiveHelpers
{
    template <typename ElementType>
        requires std::derived_from<ElementType, UObject>
    void SerializeObject(FStructuredArchive::FRecord& Record, ElementType*& Obj)
    {
        FString ClssName;
        if (!Record.GetUnderlyingArchive().IsLoading() && Obj)
        {
            UClass* Clss = Obj->GetClass();
            if (Clss)
            {
                ClssName = Clss->Name;
            }
        }

        Record << SA_VALUE("ClassName", ClssName);

        if (ClssName.empty())
        {
            return;
        }

        if (Record.GetUnderlyingArchive().IsLoading())
        {
            UClass* Clss = FindClass(ClssName);
            if (Clss && Clss->IsChildOf(ElementType::StaticClass()))
            {
                Obj = NewObject<ElementType>(nullptr, Clss);
            }
        }

        if (!Obj)
        {
            Obj = nullptr;
        }
        else
        {
            Obj->Serialize(Record);
        }
    }

    template <typename ElementType>
        requires std::derived_from<ElementType, UObject>
    void SerializeObject(FStructuredArchive::FRecord& Record, const char* Name, ElementType*& Obj)
    {
        auto ObjRecord = Record.EnterRecord(Name);
        SerializeObject(ObjRecord, Obj);
    }

    template <typename ElementType>
        requires std::derived_from<ElementType, UObject>
    void SerializeArray(FStructuredArchive::FRecord& Record, const char* Name, TArray<ElementType*>& Array)
    {
        int32 Count = Array.Num();
        auto ArrayRecord = Record.EnterArray(Name, Count);
        if (Record.GetUnderlyingArchive().IsLoading())
        {
            Array.SetNum(Count);
        }

        for (int32 i = 0; i < Count; ++i)
        {
            auto ElementRecord = ArrayRecord.EnterElement().EnterRecord();
            SerializeObject(ElementRecord, Array[i]);
        }
    }

    // Each array element is a record. The caller serializes its contents.
    template <typename ElementType, typename ElementSerializer>
    void SerializeArray(FStructuredArchive::FRecord& Record, const char* Name, TArray<ElementType>& Array, ElementSerializer Serializer)
    {
        int32 Count = Array.Num();
        auto ArrayRecord = Record.EnterArray(Name, Count);
        if (Record.GetUnderlyingArchive().IsLoading())
        {
            Array.SetNum(Count);
        }

        for (int32 i = 0; i < Count; ++i)
        {
            auto ElementRecord = ArrayRecord.EnterElement().EnterRecord();
            Serializer(ElementRecord, Array[i]);
        }
    }

    // Asset references are stored as paths, independently of UObject* formatting.
    // An empty path represents no reference; nonempty paths must already be registered.
    template <typename AssetType>
        requires std::derived_from<AssetType, UAsset>
    void SerializeAssetReference(FStructuredArchive::FRecord& Record, const char* Name, AssetType*& Asset)
    {
        FArchive& Archive = Record.GetUnderlyingArchive();

        FString AssetPath;
        if (!Archive.IsLoading() && Asset)
        {
            AssetPath = Asset->GetPath();
        }

        Record << SA_VALUE(Name, AssetPath);

        if (!Archive.IsLoading() || Archive.IsError())
        {
            return;
        }

        if (AssetPath.empty())
        {
            Asset = nullptr;
            return;
        }

        UAsset* Resolved = UAssetManager::GetAssetByPath<UAsset>(AssetPath);
        if (!Resolved)
        {
            HTR_LOG(Warning, "Load: asset '{}' not found, keeping default", AssetPath);
            return;
        }

        if (!Resolved->IsA<AssetType>())
        {
            Archive.SetError();
            return;
        }

        Asset = static_cast<AssetType*>(Resolved);
    }

    // Preserves the int32 format; enum values must be contiguous from zero to LastValue.
    template <typename T>
    void SerializeEnum(FStructuredArchive::FRecord& Parent, const char* Name, T& Value, T LastValue)
    {
        FArchive& Archive = Parent.GetUnderlyingArchive();
        int32 Number = Archive.IsLoading() ? 0 : static_cast<int32>(Value);
        Parent << SA_VALUE(Name, Number);
        if (Number < 0 || Number > static_cast<int32>(LastValue))
        {
            Archive.SetError();
            return;
        }

        if (Archive.IsLoading())
        {
            Value = static_cast<T>(Number);
        }
    }
}
