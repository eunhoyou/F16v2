#include "Task_OffensiveBFM.h"

namespace Action
{
    PortsList Task_OffensiveBFM::providedPorts()
    {
        return {
            InputPort<CPPBlackBoard*>("BB")
        };
    }

    NodeStatus Task_OffensiveBFM::tick()
    {
        Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");

        float distance = (*BB)->Distance;
        float aspectAngle = (*BB)->MyAspectAngle_Degree;
        float angleOff = (*BB)->MyAngleOff_Degree;
        float mySpeed = (*BB)->MySpeed_MS;

        std::cout << "[Task_OffensiveBFM] Distance: " << distance 
                  << "m, AspectAngle: " << aspectAngle 
                  << ", AngleOff: " << angleOff << "" << std::endl;

        // 교본 원칙: 공격 BFM 단계별 진행
        Vector3 calculated_vp;

        // 1단계: WEZ 내부 - 즉시 기총 사격 (152.4m - 914.4m)
        if (IsInWEZ(BB.value()))
        {
            std::cout << "[Task_OffensiveBFM] In WEZ - Gun tracking" << std::endl;
            calculated_vp = CalculateGunTracking(BB.value());
            (*BB)->Throttle = CalculateGunThrottle(mySpeed);
        }
        // 2단계: 기총 접근 거리 (914.4m - 1500m) - 리드 추적
        else if (distance <= 1500.0f)
        {
            std::cout << "[Task_OffensiveBFM] Gun approach range - Lead pursuit" << std::endl;
            calculated_vp = CalculateLeadPursuit(BB.value());
            (*BB)->Throttle = CalculateApproachThrottle(mySpeed, distance);
        }
        // 3단계: 중간 거리 (1500m - 3000m) - 래그 추적으로 접근
        else if (distance <= 3000.0f)
        {
            std::cout << "[Task_OffensiveBFM] Medium range - Lag pursuit" << std::endl;
            calculated_vp = CalculateLagPursuit(BB.value());
            (*BB)->Throttle = 0.85f;
        }
        // 4단계: 원거리 - 엔트리 윈도우로 진입
        else
        {
            std::cout << "[Task_OffensiveBFM] Long range - Entry window" << std::endl;
            calculated_vp = CalculateEntryWindow(BB.value());
            (*BB)->Throttle = 0.9f;
        }

        (*BB)->VP_Cartesian = calculated_vp;
        return NodeStatus::SUCCESS;
    }

    Vector3 Task_OffensiveBFM::CalculateEntryWindow(CPPBlackBoard* BB)
    {
        // 교본: 적기가 선회를 시작한 지점으로 이동 (엔트리 윈도우)
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        Vector3 targetRight = BB->TargetRightVector;
        float aspectAngle = BB->MyAspectAngle_Degree;

        // 적기의 측면으로 이동하여 터닝 룸 확보
        Vector3 sideOffset;
        if (aspectAngle > 180.0f) // 좌측 에스펙트
        {
            sideOffset = targetRight * 400.0f;
        }
        else // 우측 에스펙트
        {
            sideOffset = targetRight * -400.0f;
        }

        // 적기 후방 800m 지점을 엔트리 윈도우로 설정
        Vector3 entryPoint = targetLocation - targetForward * 800.0f + sideOffset;

        std::cout << "[CalculateEntryWindow] Moving to entry window 800m behind target" << std::endl;
        return entryPoint;
    }

    Vector3 Task_OffensiveBFM::CalculateLagPursuit(CPPBlackBoard* BB)
    {
        // 교본: 3000피트까지 래그 추적 유지
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        float distance = BB->Distance;

        // 거리에 따른 동적 래그 거리 계산
        float lagDistance = CalculateLagDistance(distance);
        
        // 적기 진행방향 뒤쪽을 추적
        Vector3 lagPoint = targetLocation - targetForward * lagDistance;

        std::cout << "[CalculateLagPursuit] Lag distance: " << lagDistance << "m" << std::endl;
        return lagPoint;
    }

    Vector3 Task_OffensiveBFM::CalculateLeadPursuit(CPPBlackBoard* BB)
    {
        // 교본: 3000피트 이내에서 리드 추적으로 전환
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        float targetSpeed = BB->TargetSpeed_MS;
        float distance = BB->Distance;

        // 간단한 리드 계산 (TOF 기반)
        float timeOfFlight = CalculateTimeOfFlight(distance);
        Vector3 leadPoint = targetLocation + targetForward * targetSpeed * timeOfFlight;

        std::cout << "[CalculateLeadPursuit] TOF: " << timeOfFlight 
                  << "s, Lead distance: " << (targetSpeed * timeOfFlight) << "m" << std::endl;
        return leadPoint;
    }

    Vector3 Task_OffensiveBFM::CalculateGunTracking(CPPBlackBoard* BB)
    {
        // 교본: WEZ 내에서 기총 추적 사격
        // 시뮬레이터에서는 단순히 적기를 향하는 것만으로도 충분
        Vector3 gunAimPoint = BB->TargetLocaion_Cartesian;

        return gunAimPoint;
    }

    float Task_OffensiveBFM::CalculateLagDistance(float distance)
    {
        // 거리의 15-25%를 래그 거리로 설정
        float lagRatio = 0.2f; // 기본 20%
        
        // 거리가 가까울수록 래그 비율 감소
        if (distance < 2000.0f)
        {
            lagRatio = 0.15f;
        }
        
        float lagDistance = distance * lagRatio;
        return std::max(100.0f, std::min(lagDistance, 500.0f)); // 100m-500m 제한
    }

    float Task_OffensiveBFM::CalculateTimeOfFlight(float distance)
    {
        // 거리에 기반한 간단한 TOF 계산
        if (distance <= 500.0f)
        {
            return 0.2f; // 500m 이하: 0.2초
        }
        else if (distance <= 1000.0f)
        {
            return 0.4f; // 1000m 이하: 0.4초
        }
        else
        {
            return 0.6f; // 그 이상: 0.6초
        }
    }

    float Task_OffensiveBFM::CalculateApproachThrottle(float mySpeed, float distance)
    {
        // 교본: 3000피트 이내에서는 스로틀로 접근율 조절
        float targetSpeed = 200.0f; // 기준 속도 200m/s

        if (distance < 500.0f)
        {
            // 매우 가까우면 속도 조절 필요
            return (mySpeed > targetSpeed) ? 0.6f : 0.8f;
        }
        else if (distance < 1000.0f)
        {
            // 중간 거리에서는 중간 추력
            return 0.75f;
        }
        else
        {
            // 접근 단계에서는 높은 추력
            return 0.9f;
        }
    }

    float Task_OffensiveBFM::CalculateGunThrottle(float mySpeed)
    {
        float result;
        if (mySpeed < 250.0f) {
            result = 1.0f;
            std::cout << "[CalculateGunThrottle] Speed=" << mySpeed << " < 250, Throttle=1.0" << std::endl;
        } else if (mySpeed > 400.0f) {
            result = 0.7f;
            std::cout << "[CalculateGunThrottle] Speed=" << mySpeed << " > 400, Throttle=0.7" << std::endl;
        } else {
            result = 0.9f;
            std::cout << "[CalculateGunThrottle] Speed=" << mySpeed << " normal range, Throttle=0.9" << std::endl;
        }
        return result;
    }

    bool Task_OffensiveBFM::IsInWEZ(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;

        // WEZ 조건: 152.4m - 914.4m, ±2도
        bool rangeValid = (distance >= WEZ_MIN_RANGE && distance <= WEZ_MAX_RANGE);
        bool angleValid = (std::abs(los) <= WEZ_MAX_ANGLE);

        return rangeValid && angleValid;
    }
}