#include "APCGameMode.h"

AAPCGameMode::AAPCGameMode()
{
	ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/VigilanteContent/Vehicles/East_APC_BTR82/BP_East_APC_BTR82.BP_East_APC_BTR82_C"));
	DefaultPawnClass = PlayerPawnBPClass.Class;
}
