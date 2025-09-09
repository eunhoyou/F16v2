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
        float altitude = (*BB)->MyLocation_Cartesian.Z;

        std::cout << "[Task_OffensiveBFM] Distance: " << distance 
                  << "m, AspectAngle: " << aspectAngle 
                  << ", AngleOff: " << angleOff 
                  << ", LOS: " << los << "" << std::endl;

        Vector3 calculated_vp;
        float offensive_Throttle;
        
        if (angleOff >= 30.0f) {
            std::cout << "[Task_OffensiveBFM] Entry window - start turn" << std::endl;
            calculated_vp = CalculateEntryWindow(BB.value());
            // offensiveThrottle = CalculateOptimalThrottle(BB.value());
        }
        else {
            std::cout << "[Task_OffensiveBFM] Lag pursuit approach" << std::endl;
            calculated_vp = CalculateLagPursuit(BB.value());
            // offensiveThrottle = CalculateOptimalThrottle(BB.value());
        }

        float current_speed = mySpeed;
        float corner_speed_upper = 250.0f; // 코너 속도 상한 (m/s)
        float corner_speed_lower = 200.0f; // 코너 속도 하한 (m/s)

        // 현재 속도가 너무 빠르면 스로틀을 줄여 코너 속도로 복귀
        if (current_speed > corner_speed_upper) {
            offensive_Throttle = 0.4f; // 아이들(Idle)에 가깝게 줄여 적극적으로 감속
        }
        // 현재 속도가 너무 느리면 스로틀을 최대로 하여 코너 속도까지 가속
        else if (current_speed < corner_speed_lower) {
            offensive_Throttle = 0.8f;
        }
        // 코너 속도 범위 내에 있다면 스로틀을 중간 정도로 유지하여 속도 유지
        else {
            offensive_Throttle = 0.6f;
        }

        (*BB)->VP_Cartesian = calculated_vp;
        (*BB)->Throttle = offensive_Throttle;
        std::cout << "[Task_DefensiveBFM] throttle: " << offensive_Throttle << std::endl; 
        return NodeStatus::SUCCESS;
    }

    Vector3 Task_OffensiveBFM::CalculateEntryWindow(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        targetForward.normalize();
        Vector3 targetRight = BB->TargetRightVector;
        targetRight.normalize();
        float aspectAngle = BB->MyAspectAngle_Degree;
        float distance = BB->Distance;

        float entryDistance = distance * 0.7f;
        Vector3 targetTail = -targetForward;
        Vector3 behindTarget = targetLocation + targetTail * entryDistance;
        
        Vector3 targetToMe = myLocation - targetLocation;
        targetToMe.normalize();
        float dotProduct = targetToMe.dot(targetRight);

        float lateralOffset = distance * 0.1f;
        if (dotProduct > 0.0f)
        {
            behindTarget = behindTarget + targetRight * lateralOffset;
            std::cout << "[CalculateEntryWindow] Right side approach" << std::endl;
        }
        else
        {
            behindTarget = behindTarget - targetRight * lateralOffset;
            std::cout << "[CalculateEntryWindow] Left side approach" << std::endl;
        }

        std::cout << "[CalculateEntryWindow] Entry distance: " << entryDistance 
                  << "m, Lateral offset: " << lateralOffset << "m" << std::endl;

        return behindTarget;
    }

    Vector3 Task_OffensiveBFM::CalculateLagPursuit(CPPBlackBoard* BB)
    {
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        targetForward.normalize();
        float distance = BB->Distance;
        
        Vector3 targetTail = -targetForward;

        float lagOffset = distance * 0.1f;
        Vector3 lagPoint = targetLocation + targetTail * lagOffset;
        
        std::cout << "[CalculateLagPursuit] lagPoint: " << lagPoint << "m" << std::endl;

        return lagPoint;
    }

    // Vector3 Task_OffensiveBFM::CalculateLeadPursuit(CPPBlackBoard* BB)
    // {
    //     Vector3 targetLocation = BB->TargetLocaion_Cartesian;
    //     Vector3 targetForward = BB->TargetForwardVector;
    //     targetForward.normalize();
    //     float distance = BB->Distance;

    //     float leadOffset = distance * 0.1f;
    //     Vector3 leadPoint = targetLocation + targetForward * leadOffset; 
        
    //     std::cout << "[CalculateLeadPursuit] leadPoint: " << leadPoint << "m" << std::endl;

    //     return leadPoint;
    // }

    Vector3 Task_OffensiveBFM::CalculatePurePursuit(CPPBlackBoard* BB)
    {
        Vector3 purePoint = BB->TargetLocaion_Cartesian;

       std::cout << "[CalculatePurePursuit] purePoint: " << purePoint << std::endl;

        return purePoint;
    }
    
    float Task_OffensiveBFM::CalculateOptimalThrottle(CPPBlackBoard* BB)
    {
        float currentThrottle = BB->Throttle;
        float currentSpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float cornerSpeed = 231.5f; // F-16 코너 속도: 약 450KCAS ≈ 231.5m/s
        
        float tempThrottle = currentThrottle;
        if (currentSpeed < targetSpeed - 20.0f)
        {
            tempThrottle = currentThrottle * 1.1f;
        }
        else if (currentSpeed > targetSpeed + 20.0f)
        {
            tempThrottle = currentThrottle * 0.9f;
        }
        else
        {
            return BB->Throttle;
        }
        
        return tempThrottle;
    }
}