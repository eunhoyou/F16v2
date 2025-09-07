#include "Task_WeaponEngagement.h"

namespace Action
{
    PortsList Task_WeaponEngagement::providedPorts()
    {
        return {
            InputPort<CPPBlackBoard*>("BB")
        };
    }

    NodeStatus Task_WeaponEngagement::tick()
    {
        Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");

        float distance = (*BB)->Distance;
        float los = (*BB)->Los_Degree;
        
        std::cout << "[Task_WeaponEngagement] Distance: " << distance 
                  << "m, LOS: " << los << "" << std::endl;

        // 시뮬레이터 특성: WEZ 내에서 즉시 타격 가능
        if (IsInWEZ(BB.value()))
        {
            std::cout << "[Task_WeaponEngagement] IN WEZ - Precision targeting" << std::endl;
            // WEZ 내: 거리는 이미 적정, 각도(±2도)만 정밀 조준
            (*BB)->VP_Cartesian = CalculateAiming(BB.value());
            (*BB)->Throttle = CalculateWEZThrottle(BB.value());
        }
        else if (IsWEZApproachable(BB.value()))
        {
            std::cout << "[Task_WeaponEngagement] WEZ approachable - Final positioning" << std::endl;
            // WEZ 진입 가능 거리: 최적 각도로 진입
            (*BB)->VP_Cartesian = CalculateWEZEntry(BB.value());
            (*BB)->Throttle = 0.85f;
        }
        else
        {
            std::cout << "[Task_WeaponEngagement] Outside WEZ - Moving to engagement range" << std::endl;
            // WEZ 밖: 신속 접근
            (*BB)->VP_Cartesian = CalculateRapidApproach(BB.value());
            (*BB)->Throttle = 1.0f;
        }

        return NodeStatus::SUCCESS;
    }

    Vector3 Task_WeaponEngagement::CalculateAiming(CPPBlackBoard* BB)
    {
        // 시뮬레이터 특성: WEZ 내에서는 단순 조준만으로 즉시 타격
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myLocation = BB->MyLocation_Cartesian;
        float los = BB->Los_Degree;
        
        // 교본: "건 크로스를 적기에게 놓는다"
        // 시뮬레이터에서는 복잡한 리드 계산 불필요 - 정확한 조준이 핵심
        Vector3 aimPoint = targetLocation;
        
        std::cout << "[CalculateAiming] Target LOS: " << los 
                  << ", Fine adjustment applied" << std::endl;
        
        return aimPoint;
    }

    Vector3 Task_WeaponEngagement::CalculateWEZEntry(CPPBlackBoard* BB)
    {
        // WEZ 진입을 위한 최적 경로
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        float distance = BB->Distance;
        
        // 교본: "적기의 6시 후방으로 이동"
        float optimalRange = (WEZ_MIN_RANGE + WEZ_MAX_RANGE) * 0.7f; // 약 640m
        
        // 적기 후방 최적 거리에 위치
        Vector3 optimalPosition = targetLocation - targetForward * optimalRange;
        
        // WEZ 각도 확보를 위한 미세 조정
        Vector3 targetRight = BB->TargetRightVector;
        float lateralOffset = optimalRange * std::sin(WEZ_MAX_ANGLE * M_PI / 180.0f) * 0.5f;
        
        // 현재 위치에서 더 유리한 쪽으로 오프셋
        Vector3 toMe = myLocation - targetLocation;
        float rightDot = toMe.dot(targetRight);
        
        if (rightDot > 0) {
            optimalPosition = optimalPosition + targetRight * lateralOffset;
        } else {
            optimalPosition = optimalPosition - targetRight * lateralOffset;
        }
        
        std::cout << "[WEZEntry] Optimal range: " << optimalRange 
                  << "m, Lateral offset: " << lateralOffset << "m" << std::endl;
        
        return optimalPosition;
    }

    Vector3 Task_WeaponEngagement::CalculateRapidApproach(CPPBlackBoard* BB)
    {
        // 원거리에서 WEZ로의 신속 접근
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        float distance = BB->Distance;
        float targetSpeed = BB->TargetSpeed_MS;

        if (distance > WEZ_MAX_RANGE * 5.0f) {
            // 매우 원거리: 요격 코스
            return CalculateInterceptCourse(BB);
        } else {
            // 중거리: 적기 후방으로 직접 기동
            float approachDistance = WEZ_MAX_RANGE * 1.5f; // WEZ 약간 밖
            
            // 적기의 예상 위치 고려
            float interceptTime = distance / (BB->MySpeed_MS + targetSpeed * 0.3f);
            Vector3 predictedTarget = targetLocation + targetForward * targetSpeed * interceptTime;
            
            Vector3 approachPoint = predictedTarget - targetForward * approachDistance;
            
            std::cout << "[RapidApproach] Intercept time: " << interceptTime 
                      << "s, Approach distance: " << approachDistance << "m" << std::endl;
            
            return approachPoint;
        }
    }

    Vector3 Task_WeaponEngagement::CalculateInterceptCourse(CPPBlackBoard* BB)
    {
        // 기본 요격 계산
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetVelocity = BB->TargetForwardVector * BB->TargetSpeed_MS;
        float mySpeed = BB->MySpeed_MS;
        float distance = BB->Distance;

        // 간단한 요격 시간 계산
        float relativeSpeed = mySpeed + BB->TargetSpeed_MS * 0.5f; // 수렴 속도
        float interceptTime = distance / relativeSpeed;
        
        // 적기의 예상 위치
        Vector3 interceptPoint = targetLocation + targetVelocity * interceptTime;
        
        std::cout << "[InterceptCourse] Intercept time: " << interceptTime 
                  << "s, Relative speed: " << relativeSpeed << "m/s" << std::endl;
        
        return interceptPoint;
    }

    float Task_WeaponEngagement::CalculateWEZThrottle(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float mySpeed = BB->MySpeed_MS;
        float los = BB->Los_Degree;
        
        // WEZ 내에서는 안정적인 조준이 우선
        if (distance < WEZ_MIN_RANGE * 1.1f) {
            // 너무 가까움 - 거리 조절 필요
            return 0.4f;
        } else if (distance > WEZ_MAX_RANGE * 0.9f) {
            // WEZ 상한 근처 - 거리 유지
            return 0.6f;
        } else {
            // WEZ 중앙 구역 - 각도 조준에 집중
            if (std::abs(los) < 1.0f) {
                // 조준 완료 - 안정 유지
                return 0.5f;
            } else {
                // 조준 중 - 기동성 확보
                return 0.7f;
            }
        }
    }

    bool Task_WeaponEngagement::IsInWEZ(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;

        bool rangeValid = (distance >= WEZ_MIN_RANGE && distance <= WEZ_MAX_RANGE);
        bool angleValid = (std::abs(los) <= WEZ_MAX_ANGLE);
        bool enemyInSight = BB->EnemyInSight;

        std::cout << "[IsInWEZ] Range: " << rangeValid << " (" << distance << "m), "
                  << "Angle: " << angleValid << " (" << los << "), "
                  << "InSight: " << enemyInSight << std::endl;

        return rangeValid && angleValid && enemyInSight;
    }

    bool Task_WeaponEngagement::IsWEZApproachable(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;

        // WEZ 진입 가능 범위: WEZ보다 약간 넓음
        bool approachableRange = (distance >= WEZ_MIN_RANGE * 0.7f && 
                                 distance <= WEZ_MAX_RANGE * 2.0f);
        bool reasonableAngle = (std::abs(los) <= WEZ_MAX_ANGLE * 3.0f);

        return approachableRange && reasonableAngle && BB->EnemyInSight;
    }
}