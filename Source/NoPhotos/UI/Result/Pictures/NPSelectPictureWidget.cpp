#include "UI/Result/Pictures/NPSelectPictureWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/Result/Pictures/NPPictureList.h"
#include "UI/Result/Pictures/NPShowPicture.h"
#include "Core/NPPlayerController.h"

void UNPSelectPictureWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(PictureListWidget))
	{
		PictureListWidget->OnPictureClicked.AddUniqueDynamic(
			this,
			&UNPSelectPictureWidget::HandlePictureClicked);
	}

	if (IsValid(ShowPictureWidget))
	{
		ShowPictureWidget->OnSelectRequested.AddUniqueDynamic(
			this,
			&UNPSelectPictureWidget::HandleSelectRequested);
	}

	if (IsValid(NextButton))
	{
		NextButton->OnClicked.AddUniqueDynamic(
			this,
			&UNPSelectPictureWidget::HandleNextButtonClicked);
	}

	UpdateSelectedPictureCountText();
	InitializeSamplePictures();
}

void UNPSelectPictureWidget::InitializePictures(
	const TArray<UTexture2D*>& InPictures)
{
	if (!IsValid(PictureListWidget))
	{
		return;
	}

	PictureListWidget->ClearPictures();

	PictureTextures.Empty();
	CurrentPictureIndex = INDEX_NONE;

	for (UTexture2D* PictureTexture : InPictures)
	{
		if (!IsValid(PictureTexture))
		{
			continue;
		}

		PictureTextures.Add(PictureTexture);
		PictureListWidget->AddPicture(PictureTexture);
	}

	if (!PictureTextures.IsEmpty())
	{
		ShowPicture(0);
	}

	UpdateSelectedPictureCountText();
}

void UNPSelectPictureWidget::HandlePictureClicked(
	const int32 PictureIndex)
{
	ShowPicture(PictureIndex);
}

void UNPSelectPictureWidget::HandleSelectRequested(
	const int32 PictureIndex)
{
	if (!IsValid(PictureListWidget)
		|| !PictureTextures.IsValidIndex(PictureIndex))
	{
		return;
	}

	const bool bWasSelected = PictureListWidget->IsPictureSelected(PictureIndex);

	//사진 선택 해제
	if (bWasSelected)
	{
		PictureListWidget->SetPictureSelected(PictureIndex, false);
		ShowPictureWidget->SetSelected(false);
		UpdateSelectedPictureCountText();
		return;
	}

	//사진 선택은 5장까지만 허용
	if (PictureListWidget->GetSelectedPictureCount() >= MaxSelectedPictureCount)
	{
		return;
	}

	PictureListWidget->SetPictureSelected(PictureIndex, true);
	ShowPictureWidget->SetSelected(true);
	UpdateSelectedPictureCountText();
}

void UNPSelectPictureWidget::HandleNextButtonClicked()
{
	const TArray<int32> SelectedPictureIndices = GetSelectedPictureIndices();

	BP_OnSelectionConfirmed(SelectedPictureIndices);

	if (ANPPlayerController* NPPlayerController=Cast<ANPPlayerController>(GetOwningPlayer()))
	{
		NPPlayerController->ShowResultUI();
	}
}

void UNPSelectPictureWidget::ShowPicture(const int32 PictureIndex)
{
	if (!PictureTextures.IsValidIndex(PictureIndex)
		|| !IsValid(ShowPictureWidget))
	{
		return;
	}

	CurrentPictureIndex = PictureIndex;

	ShowPictureWidget->SetPicture(
		PictureTextures[PictureIndex],
		PictureIndex);

	if (IsValid(PictureListWidget))
	{
		ShowPictureWidget->SetSelected(
			PictureListWidget->IsPictureSelected(PictureIndex));
	}
}

void UNPSelectPictureWidget::UpdateSelectedPictureCountText()
{
	if (!IsValid(SelectedPictureCountText)
		|| !IsValid(PictureListWidget))
	{
		return;
	}

	const int32 SelectedCount =
		PictureListWidget->GetSelectedPictureCount();

	SelectedPictureCountText->SetText(
		FText::FromString(FString::Printf(
			TEXT("고른 사진 수: %d / %d"),
			SelectedCount,
			MaxSelectedPictureCount)));
}

TArray<int32> UNPSelectPictureWidget::GetSelectedPictureIndices() const
{
	TArray<int32> SelectedIndices;

	if (!IsValid(PictureListWidget))
	{
		return SelectedIndices;
	}

	for (int32 PictureIndex = 0;
		PictureIndex < PictureTextures.Num();
		++PictureIndex)
	{
		if (PictureListWidget->IsPictureSelected(PictureIndex))
		{
			SelectedIndices.Add(PictureIndex);
		}
	}

	return SelectedIndices;
}

//테스트용
void UNPSelectPictureWidget::InitializeSamplePictures()
{
	TArray<UTexture2D*> SamplePictures;

	const TArray<FString> SamplePicturePaths =
	{
		TEXT("/Game/NoPhotos/Blueprints/UI/Result/Pictures/SamplePicture/LJJ.LJJ"),
		TEXT("/Game/NoPhotos/Blueprints/UI/Result/Pictures/SamplePicture/KMU.KMU"),
		TEXT("/Game/NoPhotos/Blueprints/UI/Result/Pictures/SamplePicture/HWH.HWH"),
		TEXT("/Game/NoPhotos/Blueprints/UI/Result/Pictures/SamplePicture/CBS.CBS"),
		TEXT("/Game/NoPhotos/Blueprints/UI/Result/Pictures/SamplePicture/0820_1.0820_1"),
		TEXT("/Game/NoPhotos/Blueprints/UI/Result/Pictures/SamplePicture/0820.0820"),
		TEXT("/Game/NoPhotos/Blueprints/UI/Result/Pictures/SamplePicture/0819.0819"),
		TEXT("/Game/NoPhotos/Blueprints/UI/Result/Pictures/SamplePicture/0818_1.0818_1")
	};

	for (const FString& PicturePath : SamplePicturePaths)
	{
		if (UTexture2D* PictureTexture =
			LoadObject<UTexture2D>(nullptr, *PicturePath))
		{
			SamplePictures.Add(PictureTexture);
		}
	}

	InitializePictures(SamplePictures);
}