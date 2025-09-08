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
            Vector3 currentVP = (*BB)->VP_Cartesian;

            float vpAltitude = currentVP.Z;
            std::cout << "[AltitudeSafetyCheck] Adjusting VP altitude to safe level (" << SAFE_ALTITUDE << "m)" << std::endl;
            currentVP.Z = SAFE_ALTITUDE;
            (*BB)->VP_Cartesian = currentVP;
            (*BB)->Throttle = 1.0f;
            return NodeStatus::SUCCESS;
        }

        if (currentAltitude <= CRITICAL_ALTITUDE)
        {
            std::cout << "[AltitudeSafetyCheck] WARNING: Altitude below " << CRITICAL_ALTITUDE << "m!" << std::endl;
            Vector3 currentVP = (*BB)->VP_Cartesian;

            float vpAltitude = currentVP.Z;
            std::cout << "[AltitudeSafetyCheck] Adjusting VP altitude to safe level (" << SAFE_ALTITUDE << "m)" << std::endl;
            currentVP.Z = SAFE_ALTITUDE;
            (*BB)->VP_Cartesian = currentVP;
            (*BB)->Throttle = 1.0f;
            return NodeStatus::SUCCESS;
        }
        return NodeStatus::SUCCESS;
    }
}