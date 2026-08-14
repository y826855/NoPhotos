#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "NPRoomCheatManager.generated.h"

UCLASS()
class NOPHOTOS_API UNPRoomCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
	void Create();
	
	// 호스트 + 방 생성
	// 두번째 게임시작 누른 사람은 게스트 + 방 참가
	
	// 방생성 , 방참가

	UFUNCTION(Exec)
	void Join();

	UFUNCTION(Exec)
	void Ready();

	UFUNCTION(Exec)
	void Start();
};
