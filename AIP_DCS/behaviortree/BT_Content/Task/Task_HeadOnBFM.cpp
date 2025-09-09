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
        float mySpeed = (*BB)->MySpeed_MS;
        float altitude = (*BB)->MyLocation_Cartesian.Z;

        std::cout << "[Task_HeadOnBFM] Distance: " << distance 
                  << "m, LOS: " << los << ", AA: " << aspectAngle 
                  << ", AO: " << angleOff << std::endl;

        float headOn_Throttle;
        if (distance > 3000.0f) {
            headOn_Throttle = 0.8f;
        }
        // 1km ~ 3km 중거리에서는 기동성 확보를 위해 코너 속도 유지 로직 적용
        else if (distance > 1000.0f) {
            float current_speed = mySpeed;
            float corner_speed_upper = 250.0f; // 코너 속도 상한 (m/s)
            float corner_speed_lower = 200.0f; // 코너 속도 하한 (m/s)

            if (current_speed > corner_speed_upper) {
                headOn_Throttle = 0.4f;
            } else if (current_speed < corner_speed_lower) {
                headOn_Throttle = 0.8f;
            } else {
                headOn_Throttle = 0.6f;
            }
        }
        // 1km 이내 근접 시에는 오버슛(지나침) 방지를 위해 감속
        else {
            headOn_Throttle = 0.7f;
        }

        (*BB)->VP_Cartesian = CalculateOffsetApproach(BB.value());
        (*BB)->Throttle = headOn_Throttle;
        std::cout << "[Task_DefensiveBFM] throttle: " << headOn_Throttle << std::endl; 
        return NodeStatus::SUCCESS;
    }
    
    Vector3 Task_HeadOnBFM::CalculateOffsetApproach(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        myRight.normalize();
        float distance = BB->Distance;
        Vector3 toTarget = targetLocation - myLocation;
        toTarget.normalize();

        float offsetDistance = distance * 0.05f;

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
        myForward.normalize();
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
        
        return tempThrottle;
    }

    // bool Task_HeadOnBFM::ShouldDisengage(CPPBlackBoard* BB)
    // {
    //     float distance = BB->Distance;
    //     float mySpeed = BB->MySpeed_MS;
    //     float targetSpeed = BB->TargetSpeed_MS;
    //     float angleOff = BB->MyAngleOff_Degree;

    //     bool significantSpeedAdvantage = (mySpeed - targetSpeed > 50.0f);
    //     bool favorableGeometry = (angleOff > 150.0f);

    //     return (significantSpeedAdvantage && favorableGeometry);
    // }
}