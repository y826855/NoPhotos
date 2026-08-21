#include "UI/Result/Pictures/NPPictureList.h"

#include "Components/ScrollBox.h"
#include "Engine/Texture2D.h"
#include "UI/Result/Pictures/NPPictureWidget.h"

void UNPPictureList::NativeConstruct()
{
	Super::NativeConstruct();

	ClearPictures();
}

void UNPPictureList::ClearPictures()
{
	if (IsValid(PictureScrollBox))
	{
		PictureScrollBox->ClearChildren();
	}

	PictureWidgets.Empty();
	SelectedPictures.Empty();
}

void UNPPictureList::AddPicture(UTexture2D* InTexture)
{
	if (!IsValid(PictureScrollBox)
		|| !IsValid(PictureWidgetClass)
		|| !IsValid(InTexture))
	{
		return;
	}

	UNPPictureWidget* NewPictureWidget =
		CreateWidget<UNPPictureWidget>(GetOwningPlayer(), PictureWidgetClass);

	if (!IsValid(NewPictureWidget))
	{
		return;
	}

	NewPictureWidget->SetPicture(InTexture);
	NewPictureWidget->SetSelected(false);
	
	NewPictureWidget->OnPictureClicked.AddDynamic(
	this,
	&UNPPictureList::HandlePictureWidgetClicked);

	PictureScrollBox->AddChild(NewPictureWidget);

	PictureWidgets.Add(NewPictureWidget);
	SelectedPictures.Add(false);
}

void UNPPictureList::SetPictureSelected(
	const int32 PictureIndex,
	const bool bSelected)
{
	if (!SelectedPictures.IsValidIndex(PictureIndex)
		|| !PictureWidgets.IsValidIndex(PictureIndex))
	{
		return;
	}

	SelectedPictures[PictureIndex] = bSelected;

	if (UNPPictureWidget* PictureWidget = PictureWidgets[PictureIndex])
	{
		PictureWidget->SetSelected(bSelected);
	}
}

bool UNPPictureList::IsPictureSelected(const int32 PictureIndex) const
{
	if (!SelectedPictures.IsValidIndex(PictureIndex))
	{
		return false;
	}

	return SelectedPictures[PictureIndex];
}

int32 UNPPictureList::GetSelectedPictureCount() const
{
	int32 SelectedPictureCount = 0;

	for (const bool bSelected : SelectedPictures)
	{
		if (bSelected)
		{
			++SelectedPictureCount;
		}
	}

	return SelectedPictureCount;
}

void UNPPictureList::HandlePictureWidgetClicked(
	UNPPictureWidget* ClickedPictureWidget)
{
	const int32 PictureIndex = PictureWidgets.IndexOfByKey(ClickedPictureWidget);

	if (PictureIndex == INDEX_NONE)
	{
		return;
	}

	OnPictureClicked.Broadcast(PictureIndex);
}