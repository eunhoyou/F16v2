#include "Task_HeadOnBFM.h"

namespace Action
{
    PortsList Task_HeadOnBFM::providedPorts()
    {
        return {
            InputPort<CPPBlackBoard*>("BB")
        };
    }

    NodeStatus Task_HeadOnBFM::tick()
    {
        Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");
        
        float distance = (*BB)->Distance;
        float los = (*BB)->Los_Degree;
        float aspectAngle = (*BB)->MyAspectAngle_Degree;
        float angleOff = (*BB)->MyAngleOff_Degree;

        std::cout << "[Task_HeadOnBFM] Distance: " << distance 
                  << "m, LOS: " << los << ", AA: " << aspectAngle 
                  << ", AO: " << angleOff << std::endl;

        if (ShouldDisengage(BB.value())) {
            std::cout << "[Task_HeadOnBFM] Escape window open - disengaging" << std::endl;
            (*BB)->VP_Cartesian = CalculateDisengagement(BB.value());
            (*BB)->Throttle = 1.0f;
        } else {
            // 단순한 오프셋 접근만 사용
            (*BB)->VP_Cartesian = CalculateOffsetApproach(BB.value());
            (*BB)->Throttle = CalculateCornerSpeedThrottle(BB.value());
        }

        return NodeStatus::SUCCESS;
    }
    
    Vector3 Task_HeadOnBFM::CalculateOffsetApproach(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        float distance = BB->Distance;
        Vector3 toTarget = (targetLocation - myLocation) / distance;
        float offsetDistance = distance * 0.2f; // 보수적 오프셋

        float rightDot = toTarget.dot(myRight);
        Vector3 offsetDirection = myRight * ((rightDot > 0) ? -1.0f : 1.0f);

        Vector3 offsetPoint = targetLocation + offsetDirection * offsetDistance;
        offsetPoint.Z = myLocation.Z;
        return offsetPoint;
    }

    Vector3 Task_HeadOnBFM::CalculateDisengagement(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;

        // 교본: "에스케이프 윈도우가 열려있을 때 신속 이탈"
        float escapeDistance = mySpeed * 2.0f;
        Vector3 escapePoint = myLocation + myForward * escapeDistance;
        escapePoint.Z = myLocation.Z + escapeDistance * 0.1f; // 상승하며 이탈
        
        std::cout << "[CalculateDisengagement] High speed escape with climb" << std::endl;
        return escapePoint;
    }

    float Task_HeadOnBFM::CalculateCornerSpeedThrottle(CPPBlackBoard* BB)
    {
        float currentThrottle = BB->Throttle;
        float currentSpeed = BB->MySpeed_MS;
        float cornerSpeed = 231.5f; // F-16 코너 속도: 약 450KCAS ≈ 231.5m/s
        
        float tempThrottle = currentThrottle;
        if (currentSpeed < cornerSpeed - 20.0f)
        {
            tempThrottle = currentThrottle * 1.2f;
        }
        else if (currentSpeed > cornerSpeed + 20.0f)
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

    bool Task_HeadOnBFM::ShouldDisengage(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float angleOff = BB->MyAngleOff_Degree;

        bool significantSpeedAdvantage = (mySpeed - targetSpeed > 50.0f);
        bool favorableGeometry = (angleOff > 150.0f);

        return (significantSpeedAdvantage && favorableGeometry);
    }
}