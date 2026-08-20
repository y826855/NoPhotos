#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NPPhotoRepository.generated.h"

class ANoPhotosGameMode;
class APlayerController;
class APlayerState;

struct FNPStoredPhoto
{
	FGuid PhotoId;
	uint16 CaptureSequence = 0;
	TWeakObjectPtr<APlayerState> Photographer;
	int32 Width = 0;
	int32 Height = 0;
	TArray<uint8> JpegData;
};

/** 서버에서 성공 판정된 사진 JPEG를 제한된 메모리 안에 보관합니다. */
UCLASS()
class NOPHOTOS_API UNPPhotoRepository : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(ANoPhotosGameMode* InGameMode);
	void AuthorizeCapture(APlayerController* Photographer, uint16 CaptureSequence);
	bool IsCaptureAuthorized(APlayerController* Photographer, uint16 CaptureSequence) const;
	bool StorePhoto(
		APlayerController* Photographer,
		const FGuid& PhotoId,
		uint16 CaptureSequence,
		int32 Width,
		int32 Height,
		TArray<uint8>&& JpegData);
	const FNPStoredPhoto* FindPhoto(const FGuid& PhotoId) const;

private:
	struct FAuthorizedCapture
	{
		TWeakObjectPtr<APlayerController> Photographer;
		uint16 CaptureSequence = 0;
	};

	void TrimOldestPhotos();

	TWeakObjectPtr<ANoPhotosGameMode> OwningGameMode;
	TArray<FAuthorizedCapture> AuthorizedCaptures;
	TMap<FGuid, FNPStoredPhoto> StoredPhotos;
	TArray<FGuid> StorageOrder;
	int64 StoredByteCount = 0;

	static constexpr int32 MaximumStoredPhotos = 50;
	static constexpr int64 MaximumStoredBytes = 16 * 1024 * 1024;
};
