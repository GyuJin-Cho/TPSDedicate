#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "TableRows.generated.h"

// 테이블 행 구조체 (원본은 Tables/Excel/*.xlsx, 작업 방법은 docs/tables.md)
// - 구조체 이름은 엑셀 파일 이름 + "Row" (WeaponTable.xlsx → FWeaponTableRow)
// - 엑셀 1행의 열 이름과 프로퍼티 이름이 같아야 한다
// - 첫 열이 키(primary key)다. DataTable 행 이름이 되고 같은 이름의 프로퍼티에도 들어간다

// 무기 목록. 키는 무기 Id (1, 2, 3 ...)
USTRUCT(BlueprintType)
struct FWeaponTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 Id = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FString WeaponName;

	// FWeaponStatTableRow의 키
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 StatId = 0;
};

// 무기 스탯. 키는 StatId (FWeaponTableRow::StatId로 찾는다)
USTRUCT(BlueprintType)
struct FWeaponStatTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponStat")
	int32 StatId = 0;

	// 몸통 기준 한 발 데미지 (부위 배율은 캐릭터 쪽)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponStat")
	float Damage = 0.f;

	// 한 발마다 조준점이 위로 튀는 각도(도) 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponStat")
	float RecoilPitchMin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponStat")
	float RecoilPitchMax = 0.f;

	// 한 발마다 좌우로 튀는 최대 각도(도). -값 ~ +값에서 랜덤
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponStat")
	float RecoilYawMax = 0.f;

	// 탄창 크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponStat")
	int32 MaxAmmo = 0;
};
