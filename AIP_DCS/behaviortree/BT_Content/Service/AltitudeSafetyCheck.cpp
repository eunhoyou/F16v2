#include "AltitudeSafetyCheck.h"

namespace Action
{
    PortsList AltitudeSafetyCheck::providedPorts()
    {
        return {
            InputPort<CPPBlackBoard*>("BB")
        };
    }

    NodeStatus AltitudeSafetyCheck::tick()
    {
        Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");
        float currentAltitude = (*BB)->MyLocation_Cartesian.Z;
   
        if (currentAltitude <= EMERGENCY_ALTITUDE)
        {
            std::cout << "[AltitudeSafetyCheck] EMERGENCY: Altitude below " << EMERGENCY_ALTITUDE << "m!" << std::endl;
            (*BB)->VP_Cartesian = CalculateEmergencyClimb(BB.value());
            (*BB)->Throttle = 1.0f;
            std::cout << "[AltitudeSafetyCheck] Emergency climb initiated - RETURNING SUCCESS" << std::endl;
            return NodeStatus::SUCCESS;
        }

        if (currentAltitude <= CRITICAL_ALTITUDE)
        {
            std::cout << "[AltitudeSafetyCheck] WARNING: Altitude below " << CRITICAL_ALTITUDE << "m!" << std::endl;
            Vector3 currentVP = (*BB)->VP_Cartesian;

            // VP의 고도가 현재보다 낮으면 안전 고도로 상승
            float vpAltitude = currentVP.Z;
            if (vpAltitude < MIN_SAFE_ALTITUDE)
            {
                std::cout << "[AltitudeSafetyCheck] Adjusting VP altitude to safe level (" << MIN_SAFE_ALTITUDE << "m)" << std::endl;
                currentVP.Z = MIN_SAFE_ALTITUDE;
                (*BB)->VP_Cartesian = currentVP;
                (*BB)->Throttle = 1.0f;
            }
            return NodeStatus::SUCCESS;
        }
    }

    Vector3 AltitudeSafetyCheck::CalculateEmergencyClimb(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;

        float currentAltitude = myLocation.Z;
        float targetAltitude = currentAltitude + 500.0f;  // 500m 상승

        Vector3 emergencyPoint = myLocation + myForward * 100.0f;
        emergencyPoint.Z = targetAltitude;
        
        std::cout << "[CalculateEmergencyClimb] Current: " << currentAltitude 
                  << "m -> Target: " << targetAltitude << "m (emergencyPoint.Z: " << emergencyPoint.Z << ")" << std::endl;

        return emergencyPoint;
    }
}