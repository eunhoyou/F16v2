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
        float currentAltitude = std::abs((*BB)->MyLocation_Cartesian.Z);
        
        // 응급 상황 (400m 이하)
        if (IsAltitudeEmergency(currentAltitude))
        {
            std::cout << "[AltitudeSafetyCheck] EMERGENCY: Altitude below " << EMERGENCY_ALTITUDE << "m!" << std::endl;
            (*BB)->VP_Cartesian = CalculateEmergencyClimb(BB.value());
            (*BB)->Throttle = 1.0f;
            std::cout << "[AltitudeSafetyCheck] Emergency climb initiated - RETURNING SUCCESS" << std::endl;
            return NodeStatus::SUCCESS;
        }

        // 위험 상황 (500m 이하)
        if (IsAltitudeCritical(currentAltitude))
        {
            std::cout << "[AltitudeSafetyCheck] WARNING: Altitude below " << CRITICAL_ALTITUDE << "m!" << std::endl;
            Vector3 currentVP = (*BB)->VP_Cartesian;

            // VP의 고도가 현재보다 낮으면 안전 고도로 상승
            float vpAltitude = std::abs(currentVP.Z);
            if (vpAltitude < MIN_SAFE_ALTITUDE)
            {
                std::cout << "[AltitudeSafetyCheck] Adjusting VP altitude to safe level (" << MIN_SAFE_ALTITUDE << "m)" << std::endl;
                currentVP.Z = -MIN_SAFE_ALTITUDE;
                (*BB)->VP_Cartesian = currentVP;
                (*BB)->Throttle = 1.0f;
            }
            return NodeStatus::SUCCESS;
        }

        // 일반적인 VP 고도 안전성 검사
        Vector3 plannedVP = (*BB)->VP_Cartesian;
        float plannedAltitude = std::abs(plannedVP.Z);

        if (plannedAltitude < MIN_SAFE_ALTITUDE)
        {
            std::cout << "[AltitudeSafetyCheck] VP altitude too low, adjusting to safe altitude (" << MIN_SAFE_ALTITUDE << "m)" << std::endl;
            plannedVP.Z = -MIN_SAFE_ALTITUDE;
            (*BB)->VP_Cartesian = plannedVP;
            (*BB)->Throttle = 1.0f;
        }

        return NodeStatus::SUCCESS;
    }

    Vector3 AltitudeSafetyCheck::CalculateEmergencyClimb(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;

        float currentAltitude = std::abs(myLocation.Z);
        
        // 🔥 더 적극적인 응급 상승
        float targetAltitude = std::max(1000.0f, currentAltitude + 500.0f);  // 최소 1000m

        // 🔥 더 가까운 거리에 VP 설정 (빠른 대응)
        Vector3 emergencyPoint = myLocation + myForward * 100.0f;  // 300m → 100m
        emergencyPoint.Z = -targetAltitude;

        std::cout << "[CalculateEmergencyClimb] Current: " << currentAltitude 
                  << "m -> Target: " << targetAltitude << "m (NED Z: " << emergencyPoint.Z << ")" << std::endl;

        return emergencyPoint;
    }

    bool AltitudeSafetyCheck::IsAltitudeCritical(float currentAltitude)
    {
        return currentAltitude <= CRITICAL_ALTITUDE;
    }

    bool AltitudeSafetyCheck::IsAltitudeEmergency(float currentAltitude)
    {
        return currentAltitude <= EMERGENCY_ALTITUDE;
    }
}