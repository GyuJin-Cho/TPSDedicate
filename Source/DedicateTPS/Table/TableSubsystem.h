#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TableSubsystem.generated.h"

// 데이터 테이블 저장소
// - 시작할 때 TablePath 아래의 DataTable을 모두 읽어 행 구조체 타입별로 등록한다
// - 행 구조체 타입과 키만으로 찾는다: Find<FWeaponTableRow>(1)
// - 서버와 클라이언트 모두에 있다 (GameInstance마다 하나)
UCLASS()
class DEDICATETPS_API UTableSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static UTableSubsystem* Get(const UObject* WorldContextObject);

	// 키로 행을 찾는다. 키는 LexToString으로 행 이름이 되는 타입(int32, FName, FString ...)
	// 테이블이나 행이 없으면 nullptr (로그를 남긴다)
	template<typename T, typename KeyType>
	const T* Find(const KeyType& Key) const
	{
		static_assert(TIsDerivedFrom<T, FTableRowBase>::IsDerived, "T must derive from FTableRowBase");

		const UDataTable* Table = FindTable(T::StaticStruct());
		if (Table == nullptr) return nullptr;

		return Table->FindRow<T>(FName(*LexToString(Key)), TEXT("UTableSubsystem::Find"), true);
	}

	// 월드 컨텍스트에서 바로 찾는다: UTableSubsystem::FindRow<FWeaponTableRow>(this, 1)
	template<typename T, typename KeyType>
	static const T* FindRow(const UObject* WorldContextObject, const KeyType& Key)
	{
		const UTableSubsystem* Subsystem = Get(WorldContextObject);
		return Subsystem ? Subsystem->Find<T>(Key) : nullptr;
	}

	// 테이블의 모든 행 (DataTable에 들어 있는 순서)
	template<typename T>
	TArray<const T*> GetAll() const
	{
		static_assert(TIsDerivedFrom<T, FTableRowBase>::IsDerived, "T must derive from FTableRowBase");

		TArray<const T*> Rows;

		const UDataTable* Table = FindTable(T::StaticStruct());
		if (Table == nullptr) return Rows;

		Table->ForeachRow<T>(TEXT("UTableSubsystem::GetAll"), [&Rows](const FName& Key, const T& Row)
		{
			Rows.Add(&Row);
		});

		return Rows;
	}

	// 테이블 에셋을 두는 폴더. Tables/import_tables.py의 DEST_PATH와 같아야 한다
	static constexpr const TCHAR* TablePath = TEXT("/Game/Data/Tables");

private:
	// 등록되지 않은 타입이면 nullptr (에러 로그)
	const class UDataTable* FindTable(const class UScriptStruct* RowStruct) const;

	void LoadTables();

	// GC 방지용
	UPROPERTY()
	TArray<TObjectPtr<class UDataTable>> LoadedTables;

	// 행 구조체 → 테이블. 구조체는 네이티브라 GC 대상이 아니다
	TMap<const class UScriptStruct*, class UDataTable*> TablesByRowStruct;
};
