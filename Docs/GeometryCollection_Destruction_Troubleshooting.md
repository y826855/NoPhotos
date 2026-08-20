# Geometry Collection 디스트럭션 문제 해결 요약

## 발생한 문제

- Static Mesh와 Geometry Collection이 겹쳐 파편이 보이지 않음
- Static Mesh를 끄면 중력과 물리도 함께 사라짐
- 파괴 순간 오브젝트가 멈추거나 랜덤한 방향으로 튐
- 파괴 판정과 Strain 로그는 나오지만 실제 조각이 분리되지 않음
- 리슨 서버에서는 파괴되지만 클라이언트에서는 파괴되지 않음
- 파편이 캐릭터와 유물의 이동을 막음

## 원인 및 해결

| 문제 | 최종 원인 | 해결 |
|---|---|---|
| 파편이 안 보임 | 기존 Static Mesh가 GC 파편을 가림 | Static Mesh와 GC의 시각 역할 분리 |
| Mesh를 끄면 물리도 꺼짐 | Static Mesh가 Root 물리 바디까지 담당 | 별도 `CollisionProxy`를 Root로 사용 |
| 파괴 순간 물리 정지 | 파괴 시 `RecreatePhysicsState()` 호출 | 해당 호출 제거, 기존 물리 상태 유지 |
| 파괴 순간 튀어 오름 | 기존 충돌체와 GC가 같은 위치에서 겹침 | 기존 충돌체가 `Destructible`을 Ignore하고 GC 활성화 전에 불필요한 충돌 제거 |
| Strain 호출 후에도 안 깨짐 | Strain이 루트 Damage Threshold보다 낮음 | `Destruction Strain > Damage Threshold`로 설정하고 다음 틱에 Strain 적용 |
| 클라이언트만 파괴 실패 | GC Root Proxy가 NetGUID 복제를 지원하지 않음 | GC 자체 복제를 끄고 서버 Reliable Multicast로 각 클라이언트에서 로컬 파괴 실행 |
| 잡자마자 파괴 | Grab Constraint의 순간 보정 충격 | 잡기 직후 일정 시간 충격 판정 무시 |
| 실행 시 Plane이 다시 생김 | 코드가 `SetStaticMesh()`로 기본 Mesh를 다시 넣음 | 런타임 Mesh 교체 코드 제거 |
| 파편이 이동을 막음 | GC 질량이 약 4.4톤으로 설정되고 파편 상호작용이 강함 | GC 전체 Mass를 약 `10~30kg`으로 낮추고 `One Way Interaction Level=1` 적용 |

## 최종 구조

### BreakableRelic

- `CollisionProxy`: 잡기, 중력, 이동, 충격 감지
- `GeometryCollection`: 외형 및 파편
- `RelicMesh`: 사용하지 않음

### BreakableGlassCase

- `CaseMesh`: 고정 프레임
- `GlassCollision`: 유리 부분 충격 감지
- `GeometryCollection`: 유리 외형 및 파편
- `LockVolume`: 열쇠 해제 감지

## 멀티플레이 처리

1. 서버가 충격과 파괴 여부를 판정
2. `bIsBroken`, `BreakLocation` 확정
3. Reliable Multicast 전송
4. 각 클라이언트가 같은 위치에서 로컬 Chaos 파괴 실행

Geometry Collection의 `Component Replicates`와 `Enable Replication`은 사용하지 않는다.

## 최종 결과

문제의 핵심은 Strain 하나가 아니라 다음 항목이 동시에 겹친 것이었다.

1. Static Mesh가 시각과 물리 Root를 동시에 담당한 구조
2. 기존 충돌체와 Geometry Collection의 중첩
3. Damage Threshold보다 낮은 Strain
4. 지원되지 않는 GC Root Proxy 네트워크 복제

최종적으로 `CollisionProxy + GeometryCollection` 역할 분리, 중첩 충돌 제거, 다음 틱 Strain 적용, 서버 Multicast 방식으로 안정화했다.
