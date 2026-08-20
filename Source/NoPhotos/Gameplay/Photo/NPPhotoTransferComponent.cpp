#include "Gameplay/Photo/NPPhotoTransferComponent.h"

#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/Photo/NPPhotoImageCodec.h"
#include "Gameplay/Photo/NPPhotoLog.h"
#include "Gameplay/Photo/NPPhotoRepository.h"
#include "NoPhotosGameMode.h"

UNPPhotoTransferComponent::UNPPhotoTransferComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.0f;
	SetIsReplicatedByDefault(true);
}

void UNPPhotoTransferComponent::BeginPlay()
{
	Super::BeginPlay();
	ImageCodec = NewObject<UNPPhotoImageCodec>(this, TEXT("PhotoTransferImageCodec"));
}

bool UNPPhotoTransferComponent::BeginUploadPhoto(
	const uint16 CaptureSequence,
	const TArray<uint8>& JpegData,
	const int32 Width,
	const int32 Height)
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (!Controller || !Controller->IsLocalController() || PendingUpload.IsSet()
		|| JpegData.IsEmpty() || JpegData.Num() > MaximumPhotoBytes)
	{
		UE_LOG(LogNPPhoto, Warning, TEXT("[PhotoTransfer] Upload rejected locally. Bytes=%d Busy=%s"), JpegData.Num(), PendingUpload.IsSet() ? TEXT("true") : TEXT("false"));
		return false;
	}

	FOutgoingTransfer Transfer;
	Transfer.Header.PhotoId = FGuid::NewGuid();
	Transfer.Header.CaptureSequence = CaptureSequence;
	Transfer.Header.TotalBytes = JpegData.Num();
	Transfer.Header.TotalChunks = FMath::DivideAndRoundUp(JpegData.Num(), ChunkSize);
	Transfer.Header.Width = Width;
	Transfer.Header.Height = Height;
	Transfer.Data = JpegData;
	if (!IsValidHeader(Transfer.Header))
	{
		return false;
	}

	PendingUpload = MoveTemp(Transfer);
	ServerBeginPhotoUpload(PendingUpload->Header);
	UE_LOG(LogNPPhoto, Log, TEXT("[PhotoTransfer] Upload started. PhotoId=%s Bytes=%d Chunks=%d"), *PendingUpload->Header.PhotoId.ToString(), PendingUpload->Header.TotalBytes, PendingUpload->Header.TotalChunks);
	return true;
}

void UNPPhotoTransferComponent::RequestPhoto(const FGuid& PhotoId)
{
	if (PhotoId.IsValid())
	{
		ServerRequestPhoto(PhotoId);
	}
}

UTexture2D* UNPPhotoTransferComponent::FindReceivedPhoto(const FGuid& PhotoId) const
{
	const TObjectPtr<UTexture2D>* FoundTexture = ReceivedPhotoTextures.Find(PhotoId);
	return FoundTexture ? FoundTexture->Get() : nullptr;
}

void UNPPhotoTransferComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PendingUpload.IsSet())
	{
		FOutgoingTransfer& Upload = PendingUpload.GetValue();
		if (Upload.NextChunkIndex < Upload.Header.TotalChunks)
		{
			TArray<uint8> Chunk;
			BuildChunk(Upload, Upload.NextChunkIndex, Chunk);
			ServerUploadPhotoChunk(Upload.Header.PhotoId, Upload.NextChunkIndex, Chunk);
			++Upload.NextChunkIndex;
		}
		else if (!Upload.bFinishSent)
		{
			Upload.bFinishSent = true;
			ServerFinishPhotoUpload(Upload.Header.PhotoId);
			PendingUpload.Reset();
		}
	}

	if (GetOwner() && GetOwner()->HasAuthority() && PendingDownload.IsSet())
	{
		FOutgoingTransfer& Download = PendingDownload.GetValue();
		if (Download.NextChunkIndex < Download.Header.TotalChunks)
		{
			TArray<uint8> Chunk;
			BuildChunk(Download, Download.NextChunkIndex, Chunk);
			ClientReceivePhotoChunk(Download.Header.PhotoId, Download.NextChunkIndex, Chunk);
			++Download.NextChunkIndex;
		}
		else if (!Download.bFinishSent)
		{
			Download.bFinishSent = true;
			ClientFinishPhotoDownload(Download.Header.PhotoId);
			PendingDownload.Reset();
		}
	}
}

bool UNPPhotoTransferComponent::IsValidHeader(const FNPPhotoTransferHeader& Header)
{
	return Header.PhotoId.IsValid()
		&& Header.TotalBytes > 0
		&& Header.TotalBytes <= MaximumPhotoBytes
		&& Header.TotalChunks == FMath::DivideAndRoundUp(Header.TotalBytes, ChunkSize)
		&& Header.Width > 0 && Header.Width <= MaximumDimension
		&& Header.Height > 0 && Header.Height <= MaximumDimension;
}

void UNPPhotoTransferComponent::BuildChunk(
	const FOutgoingTransfer& Transfer,
	const int32 ChunkIndex,
	TArray<uint8>& OutChunk)
{
	const int32 Offset = ChunkIndex * ChunkSize;
	const int32 BytesToCopy = FMath::Min(ChunkSize, Transfer.Data.Num() - Offset);
	OutChunk.Reset(BytesToCopy);
	if (BytesToCopy > 0)
	{
		OutChunk.Append(Transfer.Data.GetData() + Offset, BytesToCopy);
	}
}

void UNPPhotoTransferComponent::ServerBeginPhotoUpload_Implementation(
	const FNPPhotoTransferHeader& Header)
{
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	ANoPhotosGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANoPhotosGameMode>() : nullptr;
	UNPPhotoRepository* Repository = GameMode ? GameMode->GetPhotoRepository() : nullptr;
	if (!IsValidHeader(Header) || !Repository
		|| !Repository->IsCaptureAuthorized(Controller, Header.CaptureSequence)
		|| IncomingUpload.IsSet())
	{
		UE_LOG(LogNPPhoto, Warning, TEXT("[PhotoTransfer] Server rejected upload header. Sequence=%u"), Header.CaptureSequence);
		return;
	}

	FIncomingTransfer Transfer;
	Transfer.Header = Header;
	Transfer.Data.Reserve(Header.TotalBytes);
	IncomingUpload = MoveTemp(Transfer);
}

void UNPPhotoTransferComponent::ServerUploadPhotoChunk_Implementation(
	FGuid PhotoId,
	int32 ChunkIndex,
	const TArray<uint8>& ChunkData)
{
	if (!IncomingUpload.IsSet())
	{
		return;
	}

	FIncomingTransfer& Upload = IncomingUpload.GetValue();
	const int32 RemainingBytes = Upload.Header.TotalBytes - Upload.Data.Num();
	if (Upload.Header.PhotoId != PhotoId
		|| Upload.NextChunkIndex != ChunkIndex
		|| ChunkData.IsEmpty()
		|| ChunkData.Num() > ChunkSize
		|| ChunkData.Num() > RemainingBytes)
	{
		UE_LOG(LogNPPhoto, Warning, TEXT("[PhotoTransfer] Invalid upload chunk. Index=%d Expected=%d"), ChunkIndex, Upload.NextChunkIndex);
		IncomingUpload.Reset();
		return;
	}

	Upload.Data.Append(ChunkData);
	++Upload.NextChunkIndex;
}

void UNPPhotoTransferComponent::ServerFinishPhotoUpload_Implementation(FGuid PhotoId)
{
	if (!IncomingUpload.IsSet())
	{
		return;
	}

	FIncomingTransfer Upload = MoveTemp(IncomingUpload.GetValue());
	IncomingUpload.Reset();
	if (Upload.Header.PhotoId != PhotoId
		|| Upload.NextChunkIndex != Upload.Header.TotalChunks
		|| Upload.Data.Num() != Upload.Header.TotalBytes
		|| Upload.Data.Num() < 2
		|| Upload.Data[0] != 0xFF
		|| Upload.Data[1] != 0xD8)
	{
		UE_LOG(LogNPPhoto, Warning, TEXT("[PhotoTransfer] Incomplete upload rejected. PhotoId=%s"), *PhotoId.ToString());
		return;
	}

	ANoPhotosGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANoPhotosGameMode>() : nullptr;
	UNPPhotoRepository* Repository = GameMode ? GameMode->GetPhotoRepository() : nullptr;
	if (Repository)
	{
		Repository->StorePhoto(
			Cast<APlayerController>(GetOwner()),
			PhotoId,
			Upload.Header.CaptureSequence,
			Upload.Header.Width,
			Upload.Header.Height,
			MoveTemp(Upload.Data));
	}
}

void UNPPhotoTransferComponent::ServerRequestPhoto_Implementation(FGuid PhotoId)
{
	ANoPhotosGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANoPhotosGameMode>() : nullptr;
	const UNPPhotoRepository* Repository = GameMode ? GameMode->GetPhotoRepository() : nullptr;
	const FNPStoredPhoto* StoredPhoto = Repository ? Repository->FindPhoto(PhotoId) : nullptr;
	if (!StoredPhoto || PendingDownload.IsSet())
	{
		return;
	}

	FOutgoingTransfer Download;
	Download.Header.PhotoId = StoredPhoto->PhotoId;
	Download.Header.CaptureSequence = StoredPhoto->CaptureSequence;
	Download.Header.TotalBytes = StoredPhoto->JpegData.Num();
	Download.Header.TotalChunks = FMath::DivideAndRoundUp(Download.Header.TotalBytes, ChunkSize);
	Download.Header.Width = StoredPhoto->Width;
	Download.Header.Height = StoredPhoto->Height;
	Download.Data = StoredPhoto->JpegData;
	PendingDownload = MoveTemp(Download);
	ClientBeginPhotoDownload(PendingDownload->Header);
}

void UNPPhotoTransferComponent::ClientBeginPhotoDownload_Implementation(
	const FNPPhotoTransferHeader& Header)
{
	if (!IsValidHeader(Header) || IncomingDownload.IsSet())
	{
		return;
	}

	FIncomingTransfer Download;
	Download.Header = Header;
	Download.Data.Reserve(Header.TotalBytes);
	IncomingDownload = MoveTemp(Download);
}

void UNPPhotoTransferComponent::ClientReceivePhotoChunk_Implementation(
	FGuid PhotoId,
	int32 ChunkIndex,
	const TArray<uint8>& ChunkData)
{
	if (!IncomingDownload.IsSet())
	{
		return;
	}

	FIncomingTransfer& Download = IncomingDownload.GetValue();
	const int32 RemainingBytes = Download.Header.TotalBytes - Download.Data.Num();
	if (Download.Header.PhotoId != PhotoId
		|| Download.NextChunkIndex != ChunkIndex
		|| ChunkData.IsEmpty()
		|| ChunkData.Num() > ChunkSize
		|| ChunkData.Num() > RemainingBytes)
	{
		IncomingDownload.Reset();
		return;
	}
	Download.Data.Append(ChunkData);
	++Download.NextChunkIndex;
}

void UNPPhotoTransferComponent::ClientFinishPhotoDownload_Implementation(FGuid PhotoId)
{
	if (!IncomingDownload.IsSet())
	{
		return;
	}

	FIncomingTransfer Download = MoveTemp(IncomingDownload.GetValue());
	IncomingDownload.Reset();
	if (Download.Header.PhotoId != PhotoId
		|| Download.NextChunkIndex != Download.Header.TotalChunks
		|| Download.Data.Num() != Download.Header.TotalBytes
		|| !ImageCodec)
	{
		return;
	}

	if (UTexture2D* Texture = ImageCodec->DecodeJpegToTexture(Download.Data))
	{
		ReceivedPhotoTextures.Add(PhotoId, Texture);
		OnPhotoTextureReceived.Broadcast(PhotoId, Texture);
	}
}
