#include "MenuGameMode.h"
#include "MenuPlayerController.h"
#include "UObject/ConstructorHelpers.h"

AMenuGameMode::AMenuGameMode()
{
	DefaultPawnClass = nullptr;

	// BP 컨트롤러를 기본으로 사용, 에셋이 없으면 C++ 클래스로 대체
	static ConstructorHelpers::FClassFinder<APlayerController> ControllerBP(TEXT("/Game/Menu/BP/Controller/MenuPlayerController_BP"));
	PlayerControllerClass = ControllerBP.Succeeded() ? ControllerBP.Class : TSubclassOf<APlayerController>(AMenuPlayerController::StaticClass());
}