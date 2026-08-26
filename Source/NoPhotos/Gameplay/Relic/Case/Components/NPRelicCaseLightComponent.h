#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "NPRelicCaseLightComponent.generated.h"

class FLifetimeProperty;
class UMaterialInstanceDynamic;
class UMaterialInterface;

#if WITH_EDITOR
struct FPropertyChangedEvent;
#endif

UCLASS(ClassGroup = (Relic), meta = (BlueprintSpawnableComponent))
class NOPHOTOS_API UNPRelicCaseLightComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UNPRelicCaseLightComponent();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Relic Case Light")
	void SetLightEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Relic Case Light")
	void SetLightIntensity(float InLightIntensity);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Relic Case Light")
	void SetLightColor(FLinearColor InLightColor);

	UFUNCTION(BlueprintPure, Category = "Relic Case Light")
	bool IsLightEnabled() const { return bLightEnabled; }

	UFUNCTION(BlueprintPure, Category = "Relic Case Light")
	float GetLightIntensity() const { return LightIntensity; }

	UFUNCTION(BlueprintPure, Category = "Relic Case Light")
	FLinearColor GetLightColor() const { return LightColor; }

protected:
	virtual void OnRegister() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(
		FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UFUNCTION()
	void OnRep_LightState();

	/** 발광 표현에 사용할 머티리얼 인스턴스입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic Case Light|Material")
	TObjectPtr<UMaterialInterface> LightMaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic Case Light|Material", meta = (ClampMin = "0"))
	int32 MaterialIndex = 0;

	/** 머티리얼에 노출된 발광 세기 Scalar Parameter 이름입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic Case Light|Material")
	FName IntensityParameterName = TEXT("EmissiveIntensity");

	/** 머티리얼에 노출된 발광 색상 Vector Parameter 이름입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic Case Light|Material")
	FName ColorParameterName = TEXT("EmissiveColor");

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_LightState, BlueprintReadOnly, Category = "Relic Case Light")
	bool bLightEnabled = true;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_LightState, BlueprintReadOnly, Category = "Relic Case Light", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LightIntensity = 1.0f;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_LightState, BlueprintReadOnly, Category = "Relic Case Light")
	FLinearColor LightColor = FLinearColor::White;

private:
	void ApplyLightState();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterialInstance;
};
