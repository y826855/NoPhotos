#include "UI/GameScreen/NPBonusItemWidget.h"
#include "Components/TextBlock.h"

UNPBonusItemWidget::UNPBonusItemWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNPBonusItemWidget::SetItemName(const FString& InItemName)
{
	if (IsValid(ItemNameText))
	{
		ItemNameText->SetText(FText::FromString(InItemName));
	}
}
