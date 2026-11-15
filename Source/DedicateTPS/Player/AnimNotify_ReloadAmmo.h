#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ReloadAmmo.generated.h"

// 재장전 몽타주에서 탄창이 들어가는 시점 표시
// 노티파이 자체는 아무것도 하지 않는다. 서버가 몽타주 에셋에서 이 노티파이의 시간을 읽어 그 시점에 탄약을 채운다
// (데디 서버는 애니메이션 틱이 이동 RPC에 묶여 있어 노티파이 발생 시점을 믿을 수 없기 때문)
UCLASS(meta = (DisplayName = "Reload Ammo"))
class DEDICATETPS_API UAnimNotify_ReloadAmmo : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override { return TEXT("ReloadAmmo"); }
};
