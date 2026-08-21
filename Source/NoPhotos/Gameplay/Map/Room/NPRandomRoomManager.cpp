#include "NPRandomRoomManager.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Net/UnrealNetwork.h"

ANPRandomRoomManager::ANPRandomRoomManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TopRightSlot = CreateDefaultSubobject<UArrowComponent>(TEXT("TopRightSlot"));
	TopRightSlot->SetupAttachment(SceneRoot);

	TopLeftSlot = CreateDefaultSubobject<UArrowComponent>(TEXT("TopLeftSlot"));
	TopLeftSlot->SetupAttachment(SceneRoot);

	BottomRightSlot = CreateDefaultSubobject<UArrowComponent>(TEXT("BottomRightSlot"));
	BottomRightSlot->SetupAttachment(SceneRoot);
	BottomRightSlot->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	BottomLeftSlot = CreateDefaultSubobject<UArrowComponent>(TEXT("BottomLeftSlot"));
	BottomLeftSlot->SetupAttachment(SceneRoot);
	BottomLeftSlot->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
}

void ANPRandomRoomManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPRandomRoomManager, LayoutSeed);
}

void ANPRandomRoomManager::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		LayoutSeed = FMath::Max(FMath::Rand(), 1);
		LoadRandomRoomLayout();
	}
	else if (LayoutSeed != 0)
	{
		LoadRandomRoomLayout();
	}
}

void ANPRandomRoomManager::OnRep_LayoutSeed()
{
	LoadRandomRoomLayout();
}

void ANPRandomRoomManager::LoadRandomRoomLayout()
{
	if (bLayoutLoaded || LayoutSeed == 0 || Rooms.IsEmpty())
	{
		return;
	}

	const TArray<USceneComponent*> Slots = {
		TopRightSlot.Get(),
		TopLeftSlot.Get(),
		BottomRightSlot.Get(),
		BottomLeftSlot.Get()
	};

	TArray<int32> RoomOrder;
	RoomOrder.Reserve(Rooms.Num());
	for (int32 RoomIndex = 0; RoomIndex < Rooms.Num(); ++RoomIndex)
	{
		RoomOrder.Add(RoomIndex);
	}

	FRandomStream RandomStream(LayoutSeed);
	for (int32 Index = RoomOrder.Num() - 1; Index > 0; --Index)
	{
		RoomOrder.Swap(Index, RandomStream.RandRange(0, Index));
	}

	const int32 RoomCount = FMath::Min(RoomOrder.Num(), Slots.Num());
	for (int32 SlotIndex = 0; SlotIndex < RoomCount; ++SlotIndex)
	{
		const TSoftObjectPtr<UWorld>& Room = Rooms[RoomOrder[SlotIndex]];
		if (Room.IsNull())
		{
			continue;
		}

		const USceneComponent* Slot = Slots[SlotIndex];
		const FTransform RoomTransform(Slot->GetComponentQuat(), Slot->GetComponentLocation());

		bool bLoadSucceeded = false;
		const FString InstanceName = FString::Printf(TEXT("NP_RoomSlot_%d"), SlotIndex);
		if (ULevelStreamingDynamic* LoadedRoom = ULevelStreamingDynamic::LoadLevelInstanceBySoftObjectPtr(
			this,
			Room,
			RoomTransform,
			bLoadSucceeded,
			InstanceName);
			bLoadSucceeded && IsValid(LoadedRoom))
		{
			LoadedRooms.Add(LoadedRoom);
		}
	}

	bLayoutLoaded = true;
}
