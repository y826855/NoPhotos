#pragma once

#include "CoreMinimal.h"
#include "NPRopeTypes.generated.h"

/** 끈 한쪽 끝이 대상에 연결되어 있는지, 자유롭게 시뮬레이션되는지 나타냅니다. */
UENUM(BlueprintType)
enum class ENPRopeEndpointState : uint8
{
	Attached,
	Free
};

/**
 * 끈 끝이 따라갈 위치를 설명합니다.
 * ComponentTag가 비어 있거나 일치하는 컴포넌트가 없으면 TargetActor의 RootComponent를 사용합니다.
 */
USTRUCT(BlueprintType)
struct NOPHOTOS_API FNPRopeEndpoint
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Rope Endpoint")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rope Endpoint")
	FName ComponentTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rope Endpoint")
	FName SocketName = NAME_None;

	/** 대상 컴포넌트 또는 Socket 좌표계 기준 위치입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rope Endpoint")
	FVector LocalOffset = FVector::ZeroVector;

	/**
	 * 연결 순간의 월드 위치를 유지합니다. 연결점이 없는 액터 표면에 RopeSegment를 직접 배치할 때 사용합니다.
	 * 활성화하면 LocalOffset은 연결 시 사용하지 않습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rope Endpoint")
	bool bPreserveWorldLocationOnAttach = false;
};
