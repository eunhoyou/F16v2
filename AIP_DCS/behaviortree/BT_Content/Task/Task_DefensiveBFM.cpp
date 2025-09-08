#include "Task_DefensiveBFM.h"

namespace Action
{
    PortsList Task_DefensiveBFM::providedPorts()
    {
        return {
            InputPort<CPPBlackBoard*>("BB")
        };
    }

    NodeStatus Task_DefensiveBFM::tick()
    {
        Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");

        float distance = (*BB)->Distance;
        float los = (*BB)->Los_Degree;
        float aspectAngle = (*BB)->MyAspectAngle_Degree;
        float mySpeed = (*BB)->MySpeed_MS;

        std::cout << "[Task_DefensiveBFM] Distance: " << distance 
                  << "m, LOS: " << los 
                  << ", AspectAngle: " << aspectAngle 
                  << ", Speed: " << mySpeed << "m/s" << std::endl;
        
        Vector3 calculated_vp;
        float defensive_throttle;

        // WEZ 접근 위협
        if (IsApproachingWEZ(distance, los)) {
            calculated_vp = CalculateLiftVectorDefense(BB.value());
            defensive_throttle = 1.0f; // 최대 추력
        }
        else {
            calculated_vp = CalculateStandardDefensiveTurn(BB.value());
            defensive_throttle = CalculateDefensiveThrottle(BB.value());
        }

        (*BB)->VP_Cartesian = calculated_vp;
        (*BB)->Throttle = defensive_throttle;

        return NodeStatus::SUCCESS;
    }

    Vector3 Task_DefensiveBFM::CalculateLiftVectorDefense(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        Vector3 myRight = BB->MyRightVector;
        float mySpeed = BB->MySpeed_MS;
        
        // 교본 핵심: "적기 쪽으로 양력벡터를 놓는다"
        Vector3 toTarget = (targetLocation - myLocation);
        toTarget.normalize();
        
        // 교본: "적기를 3/9 라인 앞으로 나오게 하기"
        Vector3 turnDirection;
        if (toTarget.dot(myRight) > 0) {
            turnDirection = myRight;  // 오른쪽 선회
        } else {
            turnDirection = myRight * -1.0f;  // 왼쪽 선회
        }
        
        float defensiveDistance = mySpeed * 2.0f;
        Vector3 defensivePoint = myLocation + turnDirection * defensiveDistance;
        
        // 교본: "기수를 아래로 하고 선회"
        defensivePoint.Z = myLocation.Z - defensiveDistance * 0.1f;
        
        return defensivePoint;
    }

    Vector3 Task_DefensiveBFM::CalculateStandardDefensiveTurn(CPPBlackBoard* BB)
    {
        // 교본: "적기에게 BFM 문제를 유발하는 표준 방어 선회"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        float mySpeed = BB->MySpeed_MS;
        
        // 적기 방향으로 선회 방향 결정
        Vector3 toTarget = targetLocation - myLocation;
        toTarget.normalize();
        
        Vector3 turnDirection;
        
        if (toTarget.dot(myRight) > 0) {
            turnDirection = myRight;
        } else {
            turnDirection = myRight * -1.0f;
        }
        
        float defensiveDistance = mySpeed * 2.0f;
        Vector3 defensivePoint = myLocation + turnDirection * defensiveDistance;
        
        // 수평 방어 선회 (에너지 보존)
        defensivePoint.Z = myLocation.Z;
        
        return defensivePoint;
    }
    
    bool Task_DefensiveBFM::IsInWEZ(float distance, float los)
    {
        bool rangeCheck = (distance >= WEZ_MIN_RANGE && distance <= WEZ_MAX_RANGE);
        bool angleCheck = (std::abs(los) <= WEZ_MAX_ANGLE);
        
        return rangeCheck && angleCheck;
    }

    bool Task_DefensiveBFM::IsApproachingWEZ(float distance, float los)
    {
        float approachRange = WEZ_MAX_RANGE * 1.5f;
        float approachAngle = WEZ_MAX_ANGLE * 3.0f;
        
        bool rangeCheck = (distance >= WEZ_MIN_RANGE && distance <= approachRange);
        bool angleCheck = (std::abs(los) <= approachAngle);
        
        return rangeCheck && angleCheck;
    }

    float Task_DefensiveBFM::CalculateCornerSpeed(CPPBlackBoard* BB)
    {
        float altitude = std::abs(BB->MyLocation_Cartesian.Z);
        float baseCornerSpeed = 231.5f; // F-16 코너 속도: 약 450KCAS ≈ 231.5m/s
        
        // 고도 보정
        float altitudeBonus = (altitude / 10000.0f) * 15.0f;
        
        return baseCornerSpeed + altitudeBonus;
    }

    float Task_DefensiveBFM::CalculateDefensiveThrottle(CPPBlackBoard* BB)
    {
        float currentThrottle = BB->Throttle;
        float currentSpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float cornerSpeed = 231.5f; // F-16 코너 속도: 약 450KCAS ≈ 231.5m/s
        
        float tempThrottle = currentThrottle;
        // 교본: "코너 속도를 유지하도록 노력한다"
        if (currentSpeed < targetSpeed - 20.0f)
        {
            tempThrottle = currentThrottle * 1.2f;
        }
        else if (currentSpeed > targetSpeed + 20.0f)
        {
            tempThrottle = currentThrottle * 0.8f;
        }
        else
        {
            return BB->Throttle;
        }
        
        tempThrottle = std::max(0.0f, std::min(tempThrottle, 1.0f));
        return tempThrottle;
    }
}