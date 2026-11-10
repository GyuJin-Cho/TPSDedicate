#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MenuGameMode.generated.h"

// 메뉴 레벨 전용. 폰 없이 MenuPlayerController만 사용
UCLASS()
class DEDICATETPS_API AMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMenuGameMode();
};