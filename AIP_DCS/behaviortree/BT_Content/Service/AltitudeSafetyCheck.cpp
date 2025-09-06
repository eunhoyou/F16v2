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
                currentVP.Z = MIN_SAFE_ALTITUDE;
                (*BB)->VP_Cartesian = currentVP;
                (*BB)->Throttle = 1.0f;
            }
        }

        // 일반적인 VP 고도 안전성 검사
        Vector3 plannedVP = (*BB)->VP_Cartesian;
        float plannedAltitude = std::abs(plannedVP.Z);

        if (plannedAltitude < MIN_SAFE_ALTITUDE)
        {
            std::cout << "[AltitudeSafetyCheck] VP altitude too low, adjusting to safe altitude (" << MIN_SAFE_ALTITUDE << "m)" << std::endl;
            plannedVP.Z = MIN_SAFE_ALTITUDE;
            (*BB)->VP_Cartesian = plannedVP;
            (*BB)->Throttle = 1.0f;
        }

        return NodeStatus::SUCCESS;
    }

    Vector3 AltitudeSafetyCheck::CalculateEmergencyClimb(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myUp = BB->MyUpVector;

        float currentAltitude = std::abs(myLocation.Z);
        
        // 응급 상승: 안전 고도까지 상승 + 추가 마진
        // 현재 고도에서 200m 상승하거나, 최소 안전고도 + 100m 중 큰 값
        float emergencyMargin = 200.0f;  // 응급 상승 마진
        float safetyMargin = 100.0f;     // 안전 고도 추가 마진
        float targetAltitude = std::max(currentAltitude + emergencyMargin, MIN_SAFE_ALTITUDE + safetyMargin);

        Vector3 emergencyPoint = myLocation;
        emergencyPoint.Z = targetAltitude;

        // 약간 전진하며 상승 (실속 방지)
        Vector3 myForward = BB->MyForwardVector;
        emergencyPoint.X += myForward.X * 300.0f;
        emergencyPoint.Y += myForward.Y * 300.0f;

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