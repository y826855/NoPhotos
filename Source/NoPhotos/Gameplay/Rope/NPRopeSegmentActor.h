#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Rope/NPRopeTypes.h"
#include "NPRopeSegmentActor.generated.h"

class FLifetimeProperty;
class UCableComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnNPRopeEndpointStatesChanged,
	ENPRopeEndpointState,
	StartState,
	ENPRopeEndpointState,
	EndState);

/**
 * 두 지점 사이의 끈 한 구간을 표현합니다.
 * 게임플레이 힘이나 기믹 판정은 담당하지 않고 연결, 해제, Cable 시각화만 담당합니다.
 */
UCLASS(Blueprintable)
class NOPHOTOS_API ANPRopeSegmentActor : public AActor
{
	GENERATED_BODY()

public:
	ANPRopeSegmentActor();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Rope")
	void SetStartEndpoint(const FNPRopeEndpoint& NewEndpoint);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Rope")
	void SetEndEndpoint(const FNPRopeEndpoint& NewEndpoint);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Rope")
	void AttachStart();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Rope")
	void AttachEnd();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Rope")
	void ReleaseStart();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Rope")
	void ReleaseEnd();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Rope")
	void SetRopeVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category="Rope")
	ENPRopeEndpointState GetStartState() const { return StartState; }

	UFUNCTION(BlueprintPure, Category="Rope")
	ENPRopeEndpointState GetEndState() const { return EndState; }

	UFUNCTION(BlueprintPure, Category="Rope")
	UCableComponent* GetCableComponent() const { return CableComponent; }

	UPROPERTY(BlueprintAssignable, Category="Rope")
	FOnNPRopeEndpointStatesChanged OnEndpointStatesChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCableComponent> CableComponent;

	/** 연결 중에도 Cable 충돌을 계산합니다. 비용이 크므로 기본값은 false입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope|Physics")
	bool bEnableCollisionWhileAttached = false;

	/** 양끝이 모두 풀린 뒤에는 바닥과 충돌하며 중력으로 가라앉게 합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Rope|Physics")
	bool bEnableCollisionWhenFullyReleased = true;

	UPROPERTY(
		EditInstanceOnly,
		ReplicatedUsing=OnRep_RopeConfiguration,
		BlueprintReadOnly,
		Category="Rope|Endpoints")
	FNPRopeEndpoint StartEndpoint;

	UPROPERTY(
		EditInstanceOnly,
		ReplicatedUsing=OnRep_RopeConfiguration,
		BlueprintReadOnly,
		Category="Rope|Endpoints")
	FNPRopeEndpoint EndEndpoint;

private:
	UFUNCTION()
	void OnRep_RopeConfiguration();

	UFUNCTION()
	void OnRep_RopeVisibility();

	void ApplyRopeConfiguration();
	void ApplyStartState();
	void ApplyEndState();
	void ApplyRopePhysicsState();
	USceneComponent* ResolveEndpointComponent(
		const FNPRopeEndpoint& Endpoint) const;
	FTransform ResolveEndpointTransform(
		const FNPRopeEndpoint& Endpoint,
		const USceneComponent* Component) const;
	FVector GetCurrentStartWorldLocation() const;
	FVector GetCurrentEndWorldLocation() const;
	void NotifyStatesChanged();
	bool bHasNotifiedStates = false;
	ENPRopeEndpointState LastNotifiedStartState = ENPRopeEndpointState::Attached;
	ENPRopeEndpointState LastNotifiedEndState = ENPRopeEndpointState::Attached;

	UPROPERTY(
		ReplicatedUsing=OnRep_RopeConfiguration,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category="Rope|State",
		meta=(AllowPrivateAccess="true"))
	ENPRopeEndpointState StartState = ENPRopeEndpointState::Attached;

	UPROPERTY(
		ReplicatedUsing=OnRep_RopeConfiguration,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category="Rope|State",
		meta=(AllowPrivateAccess="true"))
	ENPRopeEndpointState EndState = ENPRopeEndpointState::Attached;

	UPROPERTY(ReplicatedUsing=OnRep_RopeConfiguration)
	FVector_NetQuantize ReleasedStartWorldLocation = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing=OnRep_RopeConfiguration)
	FVector_NetQuantize ReleasedEndWorldLocation = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing=OnRep_RopeVisibility)
	bool bRopeVisible = true;
};
