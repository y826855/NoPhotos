#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NPPhotoImageCodec.generated.h"

class UTexture2D;
class UTextureRenderTarget2D;

/** Render Target과 네트워크 전송용 JPEG 바이트 사이의 변환만 담당합니다. */
UCLASS()
class NOPHOTOS_API UNPPhotoImageCodec : public UObject
{
	GENERATED_BODY()

public:
	bool EncodeRenderTargetToJpeg(
		UTextureRenderTarget2D* RenderTarget,
		int32 Quality,
		TArray<uint8>& OutJpegData) const;

	UTexture2D* DecodeJpegToTexture(const TArray<uint8>& JpegData) const;
};
