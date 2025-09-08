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

        if (IsInWEZ(BB.value()))
        {
            (*BB)->VP_Cartesian = CalculateAiming(BB.value());
            (*BB)->Throttle = CalculateWEZThrottle(BB.value());
        }

        return NodeStatus::SUCCESS;
    }

    Vector3 Task_WeaponEngagement::CalculateAiming(CPPBlackBoard* BB)
    {
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 aimPoint = targetLocation;

        return aimPoint;
    }

    float Task_WeaponEngagement::CalculateWEZThrottle(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float mySpeed = BB->MySpeed_MS;
        float los = BB->Los_Degree;
        
        if (distance < WEZ_MIN_RANGE * 1.2) {
            return 0.4f;
        } else if (distance > WEZ_MAX_RANGE * 0.8f) {
            return 0.6f;
        } else {
            if (std::abs(los) < 1.0f) {
                return 0.5f;
            } else {
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
}