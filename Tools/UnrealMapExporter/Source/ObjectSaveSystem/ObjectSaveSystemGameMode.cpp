// Copyright Epic Games, Inc. All Rights Reserved.

#include "ObjectSaveSystemGameMode.h"
#include "ObjectSaveSystemCharacter.h"
#include "UObject/ConstructorHelpers.h"

AObjectSaveSystemGameMode::AObjectSaveSystemGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
