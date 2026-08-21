#include "UI/GameScreen/NPBonusRelicWidget.h"
#include "UI/GameScreen/NPBonusItemWidget.h"
#include "Components/VerticalBox.h"

UNPBonusRelicWidget::UNPBonusRelicWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNPBonusRelicWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitBonusItemList();
}

void UNPBonusRelicWidget::InitBonusItemList()
{
	if (!IsValid(BonusItemVerticalBox) || !IsValid(BonusItemWidgetClass)) return;

	for (int32 i = 0; i < 3; ++i)
	{
		UNPBonusItemWidget* ItemWidget = CreateWidget<UNPBonusItemWidget>(this, BonusItemWidgetClass);
		if (IsValid(ItemWidget))
		{
			ItemWidget->SetItemName(TEXT("아이템명"));
			BonusItemVerticalBox->AddChildToVerticalBox(ItemWidget);
		}
	}
}
