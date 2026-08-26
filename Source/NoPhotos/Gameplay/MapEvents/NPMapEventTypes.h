#pragma once

#include "CoreMinimal.h"
#include "NPMapEventTypes.generated.h"

/** 맵 이벤트의 성격을 구분하는 프로젝트 단위 분류입니다. */
UENUM(BlueprintType)
enum class ENPMapEventType : uint8
{
	TypeA UMETA(DisplayName = "A"),
	TypeB UMETA(DisplayName = "B"),
	Both UMETA(DisplayName = "A 또는 B")
};

/** 이벤트가 게임에 미치는 범위와 연출 규모입니다. */
UENUM(BlueprintType)
enum class ENPMapEventScale : uint8
{
	Small UMETA(DisplayName = "소"),
	Medium UMETA(DisplayName = "중"),
	Large UMETA(DisplayName = "대"),
	Colossal UMETA(DisplayName = "초대형")
};

/** 이벤트가 생성 위치 후보로 사용할 레벨 마커 종류입니다. */
UENUM(BlueprintType)
enum class ENPMapEventLocationSource : uint8
{
	Point UMETA(DisplayName = "Point"),
	Volume UMETA(DisplayName = "Volume"),
	Both UMETA(DisplayName = "Both")
};
