#include "TableSubsystem.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "DedicateTPS/Utill/Utill.h"

void UTableSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadTables();
}

void UTableSubsystem::Deinitialize()
{
	TablesByRowStruct.Empty();
	LoadedTables.Empty();

	Super::Deinitialize();
}

UTableSubsystem* UTableSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;

	return GameInstance ? GameInstance->GetSubsystem<UTableSubsystem>() : nullptr;
}

void UTableSubsystem::LoadTables()
{
	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();

	// 쿠킹 안 한 실행(에디터 바이너리 -game/-server)은 레지스트리를 비동기로 훑으므로 이 폴더만 먼저 훑는다
	if (!FPlatformProperties::RequiresCookedData())
	{
		AssetRegistry.ScanPathsSynchronous({ FString(TablePath) });
	}

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TablePath));
	Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	for (const FAssetData& Asset : Assets)
	{
		UDataTable* Table = Cast<UDataTable>(Asset.GetAsset());
		if (Table == nullptr || Table->GetRowStruct() == nullptr)
		{
			WARNING_LOG(TEXT("[Table] Failed to load %s"), *Asset.GetObjectPathString());
			continue;
		}

		const UScriptStruct* RowStruct = Table->GetRowStruct();
		if (UDataTable* const* Existing = TablesByRowStruct.Find(RowStruct))
		{
			WARNING_LOG(TEXT("[Table] %s and %s share row struct %s. Using the first one"),
				*(*Existing)->GetName(), *Table->GetName(), *RowStruct->GetName());
			continue;
		}

		LoadedTables.Add(Table);
		TablesByRowStruct.Add(RowStruct, Table);

		INFO_LOG(TEXT("[Table] Loaded %s (%s, %d rows)"), *Table->GetName(), *RowStruct->GetName(), Table->GetRowMap().Num());
	}
}

const UDataTable* UTableSubsystem::FindTable(const UScriptStruct* RowStruct) const
{
	UDataTable* const* Table = TablesByRowStruct.Find(RowStruct);
	if (Table == nullptr)
	{
		ERROR_LOG(TEXT("[Table] No table for row struct %s under %s"), *GetNameSafe(RowStruct), TablePath);
		return nullptr;
	}

	return *Table;
}
