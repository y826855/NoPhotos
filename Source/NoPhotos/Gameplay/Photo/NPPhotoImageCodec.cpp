#include "Gameplay/Photo/NPPhotoImageCodec.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Gameplay/Photo/NPPhotoLog.h"

bool UNPPhotoImageCodec::EncodeRenderTargetToJpeg(
	UTextureRenderTarget2D* RenderTarget,
	const int32 Quality,
	TArray<uint8>& OutJpegData) const
{
	OutJpegData.Reset();
	if (!IsValid(RenderTarget))
	{
		UE_LOG(LogNPPhoto, Error, TEXT("[PhotoCodec] Encode failed: invalid Render Target."));
		return false;
	}

	FImage Image;
	if (!FImageUtils::GetRenderTargetImage(RenderTarget, Image))
	{
		UE_LOG(LogNPPhoto, Error, TEXT("[PhotoCodec] Failed to read Render Target=%s"), *GetNameSafe(RenderTarget));
		return false;
	}

	TArray64<uint8> EncodedData;
	if (!FImageUtils::CompressImage(
			EncodedData,
			TEXT("jpg"),
			Image,
			FMath::Clamp(Quality, 1, 100))
		|| EncodedData.IsEmpty()
		|| EncodedData.Num() > MAX_int32)
	{
		UE_LOG(LogNPPhoto, Error, TEXT("[PhotoCodec] JPEG compression failed."));
		return false;
	}

	OutJpegData.Append(EncodedData.GetData(), static_cast<int32>(EncodedData.Num()));
	UE_LOG(
		LogNPPhoto,
		Log,
		TEXT("[PhotoCodec] JPEG encoded. Size=%d Quality=%d"),
		OutJpegData.Num(),
		FMath::Clamp(Quality, 1, 100));
	return true;
}

UTexture2D* UNPPhotoImageCodec::DecodeJpegToTexture(const TArray<uint8>& JpegData) const
{
	if (JpegData.IsEmpty())
	{
		return nullptr;
	}

	UTexture2D* Texture = FImageUtils::ImportBufferAsTexture2D(JpegData);
	if (Texture)
	{
		UE_LOG(LogNPPhoto, Log, TEXT("[PhotoCodec] JPEG decode succeeded. Size=%d Texture=%s"), JpegData.Num(), *GetNameSafe(Texture));
	}
	else
	{
		UE_LOG(LogNPPhoto, Error, TEXT("[PhotoCodec] JPEG decode failed. Size=%d"), JpegData.Num());
	}
	return Texture;
}
