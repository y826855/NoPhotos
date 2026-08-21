#include "Gameplay/Rope/NPRopeSegmentActor.h"

#include "CableComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

ANPRopeSegmentActor::ANPRopeSegmentActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CableComponent = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
	CableComponent->SetupAttachment(SceneRoot);
	CableComponent->bAttachStart = true;
	CableComponent->bAttachEnd = true;
	CableComponent->CableLength = 200.0f;
	CableComponent->NumSegments = 16;
	CableComponent->SolverIterations = 4;
	CableComponent->bEnableStiffness = false;
	CableComponent->bEnableCollision = false;
	CableComponent->CollisionFriction = 0.8f;
	CableComponent->CableGravityScale = 1.0f;

	StartEndpoint.bPreserveWorldLocationOnAttach = true;
	EndEndpoint.bPreserveWorldLocationOnAttach = false;
}

void ANPRopeSegmentActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyRopeConfiguration();
	OnRep_RopeVisibility();
}

void ANPRopeSegmentActor::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPRopeSegmentActor, StartEndpoint);
	DOREPLIFETIME(ANPRopeSegmentActor, EndEndpoint);
	DOREPLIFETIME(ANPRopeSegmentActor, StartState);
	DOREPLIFETIME(ANPRopeSegmentActor, EndState);
	DOREPLIFETIME(ANPRopeSegmentActor, ReleasedStartWorldLocation);
	DOREPLIFETIME(ANPRopeSegmentActor, ReleasedEndWorldLocation);
	DOREPLIFETIME(ANPRopeSegmentActor, bRopeVisible);
}

void ANPRopeSegmentActor::SetStartEndpoint(
	const FNPRopeEndpoint& NewEndpoint)
{
	if (!HasAuthority())
	{
		return;
	}

	StartEndpoint = NewEndpoint;
	StartState = ENPRopeEndpointState::Attached;
	ApplyRopeConfiguration();
	ForceNetUpdate();
}

void ANPRopeSegmentActor::SetEndEndpoint(
	const FNPRopeEndpoint& NewEndpoint)
{
	if (!HasAuthority())
	{
		return;
	}

	EndEndpoint = NewEndpoint;
	EndState = ENPRopeEndpointState::Attached;
	ApplyRopeConfiguration();
	ForceNetUpdate();
}

void ANPRopeSegmentActor::AttachStart()
{
	if (!HasAuthority() || StartState == ENPRopeEndpointState::Attached)
	{
		return;
	}

	StartState = ENPRopeEndpointState::Attached;
	ApplyStartState();
	ApplyRopePhysicsState();
	NotifyStatesChanged();
	ForceNetUpdate();
}

void ANPRopeSegmentActor::AttachEnd()
{
	if (!HasAuthority() || EndState == ENPRopeEndpointState::Attached)
	{
		return;
	}

	EndState = ENPRopeEndpointState::Attached;
	ApplyEndState();
	ApplyRopePhysicsState();
	NotifyStatesChanged();
	ForceNetUpdate();
}

void ANPRopeSegmentActor::ReleaseStart()
{
	if (!HasAuthority() || StartState == ENPRopeEndpointState::Free)
	{
		return;
	}

	ReleasedStartWorldLocation = GetCurrentStartWorldLocation();
	StartState = ENPRopeEndpointState::Free;
	ApplyStartState();
	ApplyRopePhysicsState();
	NotifyStatesChanged();
	ForceNetUpdate();
}

void ANPRopeSegmentActor::ReleaseEnd()
{
	if (!HasAuthority() || EndState == ENPRopeEndpointState::Free)
	{
		return;
	}

	ReleasedEndWorldLocation = GetCurrentEndWorldLocation();
	EndState = ENPRopeEndpointState::Free;
	ApplyEndState();
	ApplyRopePhysicsState();
	NotifyStatesChanged();
	ForceNetUpdate();
}

void ANPRopeSegmentActor::SetRopeVisible(bool bVisible)
{
	if (!HasAuthority() || bRopeVisible == bVisible)
	{
		return;
	}

	bRopeVisible = bVisible;
	OnRep_RopeVisibility();
	ForceNetUpdate();
}

void ANPRopeSegmentActor::OnRep_RopeConfiguration()
{
	ApplyRopeConfiguration();
}

void ANPRopeSegmentActor::OnRep_RopeVisibility()
{
	CableComponent->SetVisibility(bRopeVisible, true);
}

void ANPRopeSegmentActor::ApplyRopeConfiguration()
{
	ApplyStartState();
	ApplyEndState();
	ApplyRopePhysicsState();
	NotifyStatesChanged();
}

void ANPRopeSegmentActor::ApplyRopePhysicsState()
{
	const bool bIsFullyReleased =
		StartState == ENPRopeEndpointState::Free
		&& EndState == ENPRopeEndpointState::Free;
	CableComponent->bEnableCollision = bIsFullyReleased
		? bEnableCollisionWhenFullyReleased
		: bEnableCollisionWhileAttached;
}

void ANPRopeSegmentActor::ApplyStartState()
{
	if (StartState == ENPRopeEndpointState::Free)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		SetActorLocation(
			ReleasedStartWorldLocation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		CableComponent->bAttachStart = false;
		return;
	}

	USceneComponent* TargetComponent = ResolveEndpointComponent(StartEndpoint);
	if (!TargetComponent)
	{
		CableComponent->bAttachStart = false;
		return;
	}

	if (StartEndpoint.bPreserveWorldLocationOnAttach)
	{
		AttachToComponent(
			TargetComponent,
			FAttachmentTransformRules::KeepWorldTransform,
			StartEndpoint.SocketName);
	}
	else
	{
		AttachToComponent(
			TargetComponent,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			StartEndpoint.SocketName);
		SetActorRelativeLocation(StartEndpoint.LocalOffset);
	}

	CableComponent->bAttachStart = true;
}

void ANPRopeSegmentActor::ApplyEndState()
{
	if (EndState == ENPRopeEndpointState::Free)
	{
		CableComponent->EndLocation = CableComponent
			->GetComponentTransform()
			.InverseTransformPosition(ReleasedEndWorldLocation);
		CableComponent->bAttachEnd = false;
		return;
	}

	USceneComponent* TargetComponent = ResolveEndpointComponent(EndEndpoint);
	if (!TargetComponent)
	{
		CableComponent->bAttachEnd = false;
		return;
	}

	FVector EndOffset = EndEndpoint.LocalOffset;
	if (EndEndpoint.bPreserveWorldLocationOnAttach)
	{
		const FTransform TargetTransform = ResolveEndpointTransform(
			EndEndpoint,
			TargetComponent);
		EndOffset = TargetTransform.InverseTransformPosition(
			GetCurrentEndWorldLocation());
	}

	CableComponent->SetAttachEndToComponent(
		TargetComponent,
		EndEndpoint.SocketName);
	CableComponent->EndLocation = EndOffset;
	CableComponent->bAttachEnd = true;
}

USceneComponent* ANPRopeSegmentActor::ResolveEndpointComponent(
	const FNPRopeEndpoint& Endpoint) const
{
	if (!IsValid(Endpoint.TargetActor))
	{
		return nullptr;
	}

	if (!Endpoint.ComponentTag.IsNone())
	{
		const TArray<UActorComponent*> TaggedComponents =
			Endpoint.TargetActor->GetComponentsByTag(
				USceneComponent::StaticClass(),
				Endpoint.ComponentTag);
		for (UActorComponent* Component : TaggedComponents)
		{
			if (USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
			{
				return SceneComponent;
			}
		}
	}

	return Endpoint.TargetActor->GetRootComponent();
}

FTransform ANPRopeSegmentActor::ResolveEndpointTransform(
	const FNPRopeEndpoint& Endpoint,
	const USceneComponent* Component) const
{
	if (!Component)
	{
		return FTransform::Identity;
	}

	return Endpoint.SocketName.IsNone()
		? Component->GetComponentTransform()
		: Component->GetSocketTransform(Endpoint.SocketName, RTS_World);
}

FVector ANPRopeSegmentActor::GetCurrentStartWorldLocation() const
{
	TArray<FVector> ParticleLocations;
	CableComponent->GetCableParticleLocations(ParticleLocations);
	return ParticleLocations.IsEmpty()
		? CableComponent->GetComponentLocation()
		: ParticleLocations[0];
}

FVector ANPRopeSegmentActor::GetCurrentEndWorldLocation() const
{
	TArray<FVector> ParticleLocations;
	CableComponent->GetCableParticleLocations(ParticleLocations);
	if (!ParticleLocations.IsEmpty())
	{
		return ParticleLocations.Last();
	}

	if (USceneComponent* TargetComponent = ResolveEndpointComponent(EndEndpoint))
	{
		return ResolveEndpointTransform(EndEndpoint, TargetComponent)
			.TransformPosition(EndEndpoint.LocalOffset);
	}

	return CableComponent->GetComponentTransform().TransformPosition(
		CableComponent->EndLocation);
}

void ANPRopeSegmentActor::NotifyStatesChanged()
{
	if (bHasNotifiedStates
		&& LastNotifiedStartState == StartState
		&& LastNotifiedEndState == EndState)
	{
		return;
	}

	bHasNotifiedStates = true;
	LastNotifiedStartState = StartState;
	LastNotifiedEndState = EndState;
	OnEndpointStatesChanged.Broadcast(StartState, EndState);
}
