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
        float los = (*BB)->Los_Degree;

        std::cout << "[Task_OffensiveBFM] Distance: " << distance 
                  << "m, AspectAngle: " << aspectAngle 
                  << ", AngleOff: " << angleOff 
                  << ", LOS: " << los << "" << std::endl;

        Vector3 calculated_vp;
        float optimalThrottle;

        // 교본 원칙: 단계별 공격 BFM 진행
        if (IsInWEZ(BB.value()))
        {
            // 1단계: WEZ 내부 - 각도 최적화하여 즉시 격추
            std::cout << "[Task_OffensiveBFM] In WEZ - Angle optimization for immediate kill" << std::endl;
            calculated_vp = OptimizeWEZAngle(BB.value());
            optimalThrottle = 0.8f; // 안정적인 추력으로 정밀 조준
        }
        else if (distance <= 1500.0f)
        {
            // 2단계: WEZ 접근 거리 (914.4m - 1500m) - 리드 추적으로 WEZ 진입
            std::cout << "[Task_OffensiveBFM] WEZ approach range - Lead pursuit to WEZ" << std::endl;
            calculated_vp = CalculateLeadPursuit(BB.value());
            optimalThrottle = CalculateOptimalThrottle(BB.value(), 180.0f);
        }
        else if (distance <= 3000.0f)
        {
            // 3단계: 중간 거리 (1500m - 3000m) - 래그 추적으로 접근
            std::cout << "[Task_OffensiveBFM] Medium range - Lag pursuit approach" << std::endl;
            calculated_vp = CalculateLagPursuit(BB.value());
            optimalThrottle = CalculateOptimalThrottle(BB.value(), 220.0f);
        }
        else if (IsInsideTargetTurnCircle(BB.value()))
        {
            // 4단계: 적기 턴 서클 내부 - 직접 공격 기동
            std::cout << "[Task_OffensiveBFM] Inside target turn circle - Direct attack" << std::endl;
            calculated_vp = CalculateInterceptPoint(BB.value());
            optimalThrottle = CalculateOptimalThrottle(BB.value(), 200.0f);
        }
        else
        {
            // 5단계: 원거리 - 엔트리 윈도우로 진입
            std::cout << "[Task_OffensiveBFM] Long range - Entry window approach" << std::endl;
            calculated_vp = CalculateEntryWindow(BB.value());
            optimalThrottle = 0.9f;
        }

        (*BB)->VP_Cartesian = calculated_vp;
        (*BB)->Throttle = optimalThrottle;

        return NodeStatus::SUCCESS;
    }

    bool Task_OffensiveBFM::IsInsideTargetTurnCircle(CPPBlackBoard* BB)
    {
        // 교본: "적기의 턴 서클 안에 있는지 밖에 있는지를 자문해보아야 한다"
        float distance = BB->Distance;
        float aspectAngle = BB->MyAspectAngle_Degree;
        float targetSpeed = BB->TargetSpeed_MS;

        // 적기가 최대 G(8G)로 선회할 때의 턴 서클 반경 추정
        float estimatedTurnRadius = CalculateTurnRadius(targetSpeed, 8.0f);
        
        // 교본: "적기의 현재 선회율 대로라면 적기의 기수가 여러분을 향하거나 거의 그럴 수 있을 것 같다면"
        bool withinTurnRadius = (distance < estimatedTurnRadius * 2.5f);
        bool rearAspect = (aspectAngle < 60.0f); // 후방 에스펙트

        std::cout << "[IsInsideTargetTurnCircle] EstimatedTurnRadius: " << estimatedTurnRadius 
                  << "m, WithinRadius: " << withinTurnRadius 
                  << ", RearAspect: " << rearAspect << std::endl;

        return withinTurnRadius && rearAspect;
    }

    bool Task_OffensiveBFM::IsInWEZ(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;

        bool rangeValid = (distance >= WEZ_MIN_RANGE && distance <= WEZ_MAX_RANGE);
        bool angleValid = (std::abs(los) <= WEZ_MAX_ANGLE);

        return rangeValid && angleValid;
    }

    bool Task_OffensiveBFM::IsApproachingWEZ(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        return (distance > WEZ_MAX_RANGE && distance <= 1500.0f);
    }

    Vector3 Task_OffensiveBFM::CalculateEntryWindow(CPPBlackBoard* BB)
    {
        // 교본: "적기가 선회를 시작한 지점으로 간다"
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        Vector3 targetRight = BB->TargetRightVector;
        float aspectAngle = BB->MyAspectAngle_Degree;
        float distance = BB->Distance;

        // 교본: "엔트리 윈도우는 적기의 턴 서클 안쪽에 있다"
        float entryDistance = std::min(distance * 0.7f, 2000.0f);
        
        // 적기 후방으로 이동하되 측면 간격 확보
        Vector3 behindTarget = targetLocation - targetForward * entryDistance;
        
        // 측면 간격으로 터닝 룸 확보
        float lateralOffset = 300.0f;
        if (aspectAngle > 180.0f) // 좌측 에스펙트
        {
            behindTarget = behindTarget + targetRight * lateralOffset;
        }
        else // 우측 에스펙트
        {
            behindTarget = behindTarget - targetRight * lateralOffset;
        }

        std::cout << "[CalculateEntryWindow] Entry distance: " << entryDistance 
                  << "m, Lateral offset: " << lateralOffset << "m" << std::endl;

        return behindTarget;
    }

    Vector3 Task_OffensiveBFM::CalculateLagPursuit(CPPBlackBoard* BB)
    {
        // 교본: "3,000 피트까지는 래그 추적을 유지한다"
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        float distance = BB->Distance;
        float targetSpeed = BB->TargetSpeed_MS;

        // 동적 래그 거리 계산 (거리에 따라 조정)
        float lagDistance = distance * 0.15f; // 거리의 15%를 래그로
        lagDistance = std::max(100.0f, std::min(lagDistance, 400.0f));

        // 적기의 예상 이동을 고려한 래그 포인트
        float predictionTime = 2.0f;
        Vector3 predictedTarget = targetLocation + targetForward * targetSpeed * predictionTime;
        Vector3 lagPoint = predictedTarget - targetForward * lagDistance;

        std::cout << "[CalculateLagPursuit] Lag distance: " << lagDistance 
                  << "m, Prediction time: " << predictionTime << "s" << std::endl;

        return lagPoint;
    }

    Vector3 Task_OffensiveBFM::CalculateLeadPursuit(CPPBlackBoard* BB)
    {
        // 교본: "3,000 피트 이내에서 리드를 당기고 기총 사격을 준비한다"
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        float targetSpeed = BB->TargetSpeed_MS;
        float distance = BB->Distance;

        // WEZ 진입을 위한 리드 계산
        float leadTime = distance / 800.0f; // 접근 속도 고려
        leadTime = std::max(0.5f, std::min(leadTime, 2.0f));

        Vector3 leadPoint = targetLocation + targetForward * targetSpeed * leadTime;

        std::cout << "[CalculateLeadPursuit] Lead time: " << leadTime 
                  << "s, Lead distance: " << (targetSpeed * leadTime) << "m" << std::endl;

        return leadPoint;
    }

    Vector3 Task_OffensiveBFM::CalculateGunTracking(CPPBlackBoard* BB)
    {
        // WEZ 내에서는 단순히 적기를 조준 (즉시 타격 시스템)
        return BB->TargetLocaion_Cartesian;
    }

    Vector3 Task_OffensiveBFM::OptimizeWEZAngle(CPPBlackBoard* BB)
    {
        // WEZ 내에서 각도 최적화 (±2도 내로 조정)
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float los = BB->Los_Degree;

        // 각도가 이미 적정하면 그대로 추적
        if (std::abs(los) <= WEZ_MAX_ANGLE)
        {
            return targetLocation;
        }

        // 각도 조정이 필요한 경우 미세 조정
        Vector3 toTarget = targetLocation - myLocation;
        float distance = BB->Distance;
        
        // WEZ 각도 내로 들어가기 위한 미세 조정
        float angleAdjustment = (los > 0) ? -1.0f : 1.0f;
        Vector3 adjustedPoint = targetLocation + BB->MyRightVector * (distance * 0.035f * angleAdjustment);

        std::cout << "[OptimizeWEZAngle] LOS: " << los 
                  << ", Angle adjustment: " << angleAdjustment << std::endl;

        return adjustedPoint;
    }

    Vector3 Task_OffensiveBFM::CalculateInterceptPoint(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetVelocity = BB->TargetForwardVector * BB->TargetSpeed_MS;
        float mySpeed = BB->MySpeed_MS;
        float distance = BB->Distance;

        // 간단한 요격점 계산
        float interceptTime = distance / (mySpeed + BB->TargetSpeed_MS * 0.5f);
        Vector3 interceptPoint = targetLocation + targetVelocity * interceptTime;

        return interceptPoint;
    }

    float Task_OffensiveBFM::CalculateCornerSpeed(CPPBlackBoard* BB)
    {
        // F-16 코너 속도: 약 450KCAS ≈ 130m/s (교본 기준)
        float altitude = std::abs(BB->MyLocation_Cartesian.Z);
        float baseCornerSpeed = 130.0f;
        
        // 고도별 보정
        float altitudeBonus = (altitude / 10000.0f) * 15.0f;
        
        return baseCornerSpeed + altitudeBonus;
    }

    float Task_OffensiveBFM::CalculateTurnRadius(float speed, float gLoad)
    {
        // 교본 공식: TR = V²/(g*G)
        return (speed * speed) / (9.81f * gLoad);
    }

    float Task_OffensiveBFM::CalculateOptimalThrottle(CPPBlackBoard* BB, float targetSpeed)
    {
        float currentSpeed = BB->MySpeed_MS;
        float cornerSpeed = CalculateCornerSpeed(BB);
        
        // 교본: "코너 속도를 유지하도록 노력한다"
        if (currentSpeed < targetSpeed - 20.0f)
        {
            return 1.0f; // 가속
        }
        else if (currentSpeed > targetSpeed + 20.0f)
        {
            return 0.6f; // 감속
        }
        else
        {
            return 0.8f; // 유지
        }
    }
}