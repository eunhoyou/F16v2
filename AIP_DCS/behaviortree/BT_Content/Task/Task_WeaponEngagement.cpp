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

        // WEZ 조건 확인
        if (IsInWEZ(BB.value()))
        {
            std::cout << "[Task_WeaponEngagement] In WEZ - Engaging target" << std::endl;
            // WEZ 내에서 정밀 조준 - 리드 추적 적용
            (*BB)->VP_Cartesian = CalculateGunTrackingPoint(BB.value());
            (*BB)->Throttle = CalculateEngagementThrottle(BB.value());
        }
        else if (IsApproachingWEZ(BB.value()))
        {
            std::cout << "[Task_WeaponEngagement] Approaching WEZ - Final approach" << std::endl;
            // WEZ 진입을 위한 최종 접근
            (*BB)->VP_Cartesian = CalculateWEZApproach(BB.value());
            (*BB)->Throttle = 0.9f; // 높은 추력으로 빠른 접근
        }
        else
        {
            std::cout << "[Task_WeaponEngagement] Outside WEZ - Moving to engagement range" << std::endl;
            // WEZ 밖에서 진입 기동
            (*BB)->VP_Cartesian = CalculateWEZEntry(BB.value());
            (*BB)->Throttle = 1.0f; // 최대 추력으로 신속 접근
        }

        return NodeStatus::SUCCESS;
    }

    Vector3 Task_WeaponEngagement::CalculateGunTrackingPoint(CPPBlackBoard* BB)
    {
        // 교본: 기총 사격시 리드 추적 필요 (TOF 0.5-1.5초)
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        Vector3 myLocation = BB->MyLocation_Cartesian;
        float targetSpeed = BB->TargetSpeed_MS;
        float distance = BB->Distance;

        // 동적 TOF 계산 (거리 기반)
        float timeOfFlight = CalculateTimeOfFlight(distance);
        
        // 리드 포인트 계산 - 적기의 예상 위치
        Vector3 leadPoint = targetLocation + targetForward * targetSpeed * timeOfFlight;

        // 교본의 기총 사격 기법 적용
        // "건 크로스를 적기의 전방에 놓고, 펀넬로 조준한다"
        Vector3 trackingPoint = ApplyGunLeadCorrection(leadPoint, BB);

        std::cout << "[CalculateGunTrackingPoint] TOF: " << timeOfFlight 
                  << "s, Lead distance: " << (targetSpeed * timeOfFlight) << "m" << std::endl;

        return trackingPoint;
    }

    Vector3 Task_WeaponEngagement::CalculateWEZApproach(CPPBlackBoard* BB)
    {
        // WEZ 진입 직전 - 정확한 각도와 거리로 접근
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        float distance = BB->Distance;

        // WEZ 최적 거리로 접근 (교본: 2500-4000피트 사이)
        float optimalRange = (WEZ_MIN_RANGE + WEZ_MAX_RANGE) * 0.6f; // 약 640m
        
        // 적기 후방으로 접근하되 WEZ 각도 내로
        Vector3 toTarget = (targetLocation - myLocation) / distance;
        Vector3 approachPoint = targetLocation - toTarget * optimalRange;

        // 약간의 오프셋으로 WEZ 각도 확보
        Vector3 targetRight = BB->TargetRightVector;
        float offsetDistance = optimalRange * std::sin(WEZ_MAX_ANGLE * M_PI / 180.0f);
        approachPoint = approachPoint + targetRight * offsetDistance * 0.5f;

        std::cout << "[CalculateWEZApproach] Optimal range: " << optimalRange << "m" << std::endl;

        return approachPoint;
    }

    Vector3 Task_WeaponEngagement::CalculateWEZEntry(CPPBlackBoard* BB)
    {
        // 원거리에서 WEZ 진입을 위한 기동
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        float distance = BB->Distance;

        if (distance > WEZ_MAX_RANGE * 3.0f)
        {
            // 매우 원거리 - 요격 코스로 접근
            return CalculateInterceptCourse(BB);
        }
        else
        {
            // 중거리 - 적기 후방으로 기동
            float entryDistance = WEZ_MAX_RANGE * 1.2f; // WEZ 약간 밖에서 대기
            Vector3 entryPoint = targetLocation - targetForward * entryDistance;
            
            std::cout << "[CalculateWEZEntry] Entry distance: " << entryDistance << "m" << std::endl;
            return entryPoint;
        }
    }

    Vector3 Task_WeaponEngagement::CalculateInterceptCourse(CPPBlackBoard* BB)
    {
        // 기본 요격 코스 계산
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetVelocity = BB->TargetForwardVector * BB->TargetSpeed_MS;
        float mySpeed = BB->MySpeed_MS;

        // 간단한 요격 계산
        Vector3 relativePos = targetLocation - myLocation;
        float distance = BB->Distance;
        float interceptTime = distance / (mySpeed + BB->TargetSpeed_MS * 0.5f);
        
        Vector3 interceptPoint = targetLocation + targetVelocity * interceptTime;
        
        return interceptPoint;
    }

    Vector3 Task_WeaponEngagement::ApplyGunLeadCorrection(Vector3 basePoint, CPPBlackBoard* BB)
    {
        // 교본의 기총 조준 보정 적용
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetVelocity = BB->TargetForwardVector * BB->TargetSpeed_MS;
        float distance = BB->Distance;

        // 교본: "G를 늦추면서 사격한다" - 기동 중 사격 고려
        Vector3 correctedPoint = basePoint;

        // 거리에 따른 리드 보정
        if (distance < 400.0f)
        {
            // 근거리에서는 리드를 줄임
            correctedPoint = correctedPoint - targetVelocity * 0.1f;
        }

        return correctedPoint;
    }

    float Task_WeaponEngagement::CalculateTimeOfFlight(float distance)
    {
        // 교본 기준 TOF 계산 (0.5-1.5초)
        // 20mm 탄환 속도 약 1000m/s 기준
        const float BULLET_VELOCITY = 1000.0f;
        
        float baseTOF = distance / BULLET_VELOCITY;
        
        // 최소 0.1초, 최대 1.5초로 제한
        return std::max(0.1f, std::min(baseTOF, 1.5f));
    }

    float Task_WeaponEngagement::CalculateEngagementThrottle(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float mySpeed = BB->MySpeed_MS;
        
        // 교본: "스로틀로 접근율을 조정한다"
        if (distance < WEZ_MIN_RANGE * 1.2f)
        {
            // 너무 가까움 - 추력 감소
            return 0.3f;
        }
        else if (distance > WEZ_MAX_RANGE * 0.8f)
        {
            // WEZ 상한에 가까움 - 추력 유지
            return 0.7f;
        }
        else
        {
            // WEZ 내 최적 거리 - 속도 유지
            return (mySpeed > 180.0f) ? 0.5f : 0.8f;
        }
    }

    bool Task_WeaponEngagement::IsInWEZ(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;

        // WEZ 조건 확인
        bool rangeValid = (distance >= WEZ_MIN_RANGE && distance <= WEZ_MAX_RANGE);
        bool angleValid = (std::abs(los) <= WEZ_MAX_ANGLE);
        bool enemyInSight = BB->EnemyInSight;

        std::cout << "[IsInWEZ] Range: " << rangeValid << " (" << distance << "m), "
                  << "Angle: " << angleValid << " (" << los << "), "
                  << "InSight: " << enemyInSight << std::endl;

        return rangeValid && angleValid && enemyInSight;
    }

    bool Task_WeaponEngagement::IsApproachingWEZ(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;

        // WEZ 접근 조건 (WEZ보다 약간 넓은 범위)
        bool approachRange = (distance >= WEZ_MIN_RANGE * 0.8f && 
                             distance <= WEZ_MAX_RANGE * 1.3f);
        bool approachAngle = (std::abs(los) <= WEZ_MAX_ANGLE * 2.0f);

        return approachRange && approachAngle && BB->EnemyInSight;
    }
}