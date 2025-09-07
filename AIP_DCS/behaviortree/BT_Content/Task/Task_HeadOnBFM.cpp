#include "Task_HeadOnBFM.h"
#include <algorithm> // For std::min/max
#include <cmath>     // For std::abs, M_PI, sin, cos

// Define M_PI if it's not available (for MSVC)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
        Optional<CPPBlackBoard*> BB_opt = getInput<CPPBlackBoard*>("BB");
        if (!BB_opt || !(*BB_opt)) {
            return NodeStatus::FAILURE;
        }
        CPPBlackBoard* BB = BB_opt.value();
        
        float distance = BB->Distance;
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float myAltitude = static_cast<float>(std::abs(BB->MyLocation_Cartesian.Z));
        float los = BB->Los_Degree;
        float aspectAngle = BB->MyAspectAngle_Degree;

        std::cout << "[Task_HeadOnBFM] Distance: " << distance 
                  << "m, MySpeed: " << mySpeed 
                  << "m/s, TargetSpeed: " << targetSpeed 
                  << "m/s, LOS: " << los << ", AA: " << aspectAngle << "" << std::endl;

        if (ShouldDisengage(BB)) {
            std::cout << "[Task_HeadOnBFM] Escape window open - disengaging" << std::endl;
            BB->VP_Cartesian = CalculateDisengagement(BB);
            BB->Throttle = 1.0f;
            return NodeStatus::SUCCESS;
        }

        if (distance > 6000.0f) {
            std::cout << "[Task_HeadOnBFM] Long range - tactical approach" << std::endl;
            BB->VP_Cartesian = CalculateTacticalApproach(BB);
            BB->Throttle = 0.85f;
        }
        else if (distance > 3000.0f && distance <= 6000.0f) {
            if (ShouldInitiateLeadTurn(BB)) {
                std::cout << "[Task_HeadOnBFM] Medium range - lead turn" << std::endl;
                BB->VP_Cartesian = CalculateLeadTurn(BB);
                BB->Throttle = 0.8f;
            } else {
                std::cout << "[Task_HeadOnBFM] Medium range - offset approach" << std::endl;
                BB->VP_Cartesian = CalculateOffsetApproach(BB);
                BB->Throttle = 0.85f;
            }
        }
        else if (distance > 1500.0f && distance <= 3000.0f) {
            float energyState = CalculateEnergyState(BB);
            
            if (energyState > 0.2f) {
                std::cout << "[Task_HeadOnBFM] Energy advantage - aggressive slice" << std::endl;
                BB->VP_Cartesian = CalculateAggressiveSlice(BB);
                BB->Throttle = 0.7f;
            } else if (energyState < -0.2f) {
                std::cout << "[Task_HeadOnBFM] Energy disadvantage - defensive maneuver" << std::endl;
                BB->VP_Cartesian = CalculateDefensiveManeuver(BB);
                BB->Throttle = 0.9f;
            } else {
                std::cout << "[Task_HeadOnBFM] Neutral energy - standard maneuver" << std::endl;
                BB->VP_Cartesian = CalculateStandardManeuver(BB);
                BB->Throttle = 0.8f;
            }
        }
        else {
            std::cout << "[Task_HeadOnBFM] Close range - preparing for BFM transition" << std::endl;
            BB->VP_Cartesian = CalculateBFMTransition(BB);
            BB->Throttle = 0.85f;
        }

        return NodeStatus::SUCCESS;
    }

    Vector3 Task_HeadOnBFM::CalculateTacticalApproach(CPPBlackBoard* BB) {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        Vector3 targetRight = BB->TargetRightVector;
        float distance = BB->Distance;

        // 제한된 오프셋 (과도한 기동 방지)
        float offsetAngle = 20.0f * M_PI / 180.0f; // 30도 → 20도로 감소
        float approachDistance = std::min(distance * 0.6f, 2000.0f); // 최대 거리 제한

        // 더 안정적인 측면 선택
        Vector3 toMe = myLocation - targetLocation;
        float rightDot = toMe.dot(targetRight);
        float sideMultiplier = (rightDot > 0) ? 1.0f : -1.0f;

        // 계산된 오프셋이 합리적인 범위 내인지 검증
        Vector3 sideOffset = targetRight * sideMultiplier * approachDistance * std::sin(offsetAngle);
        Vector3 frontOffset = targetForward * (-approachDistance * std::cos(offsetAngle));
        Vector3 tacticalPoint = targetLocation + frontOffset + sideOffset;

        // VP 유효성 검증
        float vpDistance = myLocation.distance(tacticalPoint);
        if (vpDistance > distance * 2.0f) {
            // VP가 너무 멀면 더 보수적으로 설정
            tacticalPoint = myLocation + (targetLocation - myLocation) * 0.7f;
        }

        // 고도 관리 개선
        float altitudeDiff = myLocation.Z - targetLocation.Z;
        if (altitudeDiff > -200.0f) { // 고도 우위가 부족하면
            tacticalPoint.Z = myLocation.Z - 300.0f; // 300m 상승
        } else {
            tacticalPoint.Z = myLocation.Z; // 현재 고도 유지
        }

        return tacticalPoint;
    }

    Vector3 Task_HeadOnBFM::CalculateOffsetApproach(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        float distance = BB->Distance;

        Vector3 toTarget = (targetLocation - myLocation) / distance;
        float offsetDistance = distance * 0.25f;

        float rightDot = toTarget.dot(myRight);
        Vector3 offsetDirection = myRight * ((rightDot > 0) ? -1.0f : 1.0f);

        Vector3 offsetPoint = targetLocation + offsetDirection * offsetDistance;
        offsetPoint.Z = myLocation.Z;

        return offsetPoint;
    }

    Vector3 Task_HeadOnBFM::CalculateAggressiveSlice(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        Vector3 myRight = BB->MyRightVector;
        float mySpeed = BB->MySpeed_MS;
        float distance = BB->Distance;

        Vector3 toTarget = (targetLocation - myLocation) / distance;
        float dotRight = toTarget.dot(myRight);
        Vector3 sliceDirection = (dotRight > 0) ? myRight : myRight * -1.0f;

        float turnRadius = (mySpeed * mySpeed) / (9.81f * 7.0f);
        float sliceDistance = turnRadius * 1.5f;

        Vector3 slicePoint = myLocation + sliceDirection * sliceDistance + myForward * (mySpeed * 2.5f);
        slicePoint.Z = myLocation.Z - 400.0f; // NED 좌표계에서 400m 상승

        std::cout << "[AggressiveSlice] 7G turn, radius: " << turnRadius << "m" << std::endl;
        return slicePoint;
    }

    Vector3 Task_HeadOnBFM::CalculateDefensiveManeuver(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;
        float myAltitude = std::abs((float)myLocation.Z);

        if (myAltitude < 6000.0f && mySpeed > 250.0f) {
            float climbHeight = 500.0f;
            Vector3 climbPoint = myLocation + myForward * (mySpeed * 2.0f);
            climbPoint.Z = myLocation.Z - climbHeight;  // NED 좌표계에서 상승
            std::cout << "[DefensiveManeuver] Climbing 500m for energy" << std::endl;
            return climbPoint;
        } else {
            Vector3 conservePoint = myLocation + myForward * (mySpeed * 3.0f);
            conservePoint.Z = myLocation.Z;  // 수평 비행
            std::cout << "[DefensiveManeuver] Level flight for energy conservation" << std::endl;
            return conservePoint;
        }
    }

    Vector3 Task_HeadOnBFM::CalculateStandardManeuver(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;
        float distance = BB->Distance;

        Vector3 toTarget = (targetLocation - myLocation) / distance;
        float dotRight = toTarget.dot(myRight);
        Vector3 turnDirection = (dotRight > 0) ? myRight : myRight * -1.0f;

        float turnRadius = (mySpeed * mySpeed) / (9.81f * 6.0f);
        float turnDistance = turnRadius * 1.2f;

        Vector3 standardPoint = myLocation + turnDirection * turnDistance + myForward * (mySpeed * 2.0f);
        standardPoint.Z = myLocation.Z;

        return standardPoint;
    }

    Vector3 Task_HeadOnBFM::CalculateBFMTransition(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;

        Vector3 transitionPoint = myLocation + myForward * (mySpeed * 1.5f);
        transitionPoint.Z = myLocation.Z;

        return transitionPoint;
    }

    Vector3 Task_HeadOnBFM::CalculateLeadTurn(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        Vector3 myRight = BB->MyRightVector;
        float targetSpeed = BB->TargetSpeed_MS;
        float distance = BB->Distance;

        float leadTime = std::min(distance / 800.0f, 4.0f);
        Vector3 predictedTargetPos = targetLocation + targetForward * targetSpeed * leadTime;

        Vector3 toPredict = (predictedTargetPos - myLocation);
        // **FIXED ERROR 2**
        // Use distance() method instead of non-existent magnitude()
        float predictDistance = myLocation.distance(predictedTargetPos);
        if (predictDistance > 0.0001f) {
            toPredict = toPredict / predictDistance;
        }
        
        float dotRight = toPredict.dot(myRight);
        Vector3 leadDirection = (dotRight > 0) ? myRight : myRight * -1.0f;

        float leadDistance = std::min(distance * 0.4f, 1200.0f);
        Vector3 leadTurnPoint = myLocation + leadDirection * leadDistance;

        std::cout << "[CalculateLeadTurn] Lead time: " << leadTime 
                  << "s, Lead distance: " << leadDistance << "m" << std::endl;

        return leadTurnPoint;
    }

    Vector3 Task_HeadOnBFM::CalculateDisengagement(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;

        float escapeDistance = mySpeed * 6.0f;
        Vector3 escapePoint = myLocation + myForward * escapeDistance;
        escapePoint.Z = myLocation.Z - 300.0f;  // NED 좌표계에서 300m 상승
        std::cout << "[CalculateDisengagement] Climbing escape" << std::endl;
        return escapePoint;
    }

    float Task_HeadOnBFM::CalculateEnergyState(CPPBlackBoard* BB)
    {
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float myAltitude = std::abs((float)BB->MyLocation_Cartesian.Z);
        float targetAltitude = std::abs((float)BB->TargetLocaion_Cartesian.Z);

        float myEnergy = 0.5f * mySpeed * mySpeed + 9.81f * myAltitude;
        float targetEnergy = 0.5f * targetSpeed * targetSpeed + 9.81f * targetAltitude;
        float avgEnergy = (myEnergy + targetEnergy) * 0.5f;

        if (avgEnergy < 1.0f) return 0.0f; // Avoid division by near-zero
        return (myEnergy - targetEnergy) / avgEnergy;
    }

    bool Task_HeadOnBFM::ShouldInitiateLeadTurn(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;
        float aspectAngle = BB->MyAspectAngle_Degree;
        float angleOff = BB->MyAngleOff_Degree;

        bool distanceCheck = (distance > 2500.0f && distance < 5000.0f);
        bool angleCheck = (std::abs(los) < 60.0f);
        bool headOnCheck = (aspectAngle > 150.0f || aspectAngle < 30.0f);
        bool converging = (angleOff > 90.0f);

        bool shouldLead = distanceCheck && angleCheck && headOnCheck && converging;

        std::cout << "[ShouldInitiateLeadTurn] Dist:" << distanceCheck 
                  << " Angle:" << angleCheck << " HeadOn:" << headOnCheck 
                  << " Conv:" << converging << " -> " << shouldLead << std::endl;

        return shouldLead;
    }

    bool Task_HeadOnBFM::ShouldDisengage(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float angleOff = BB->MyAngleOff_Degree;
        float myAltitude = std::abs((float)BB->MyLocation_Cartesian.Z);

        bool significantSpeedAdvantage = (mySpeed - targetSpeed > 80.0f);
        bool favorableGeometry = (angleOff > 150.0f && distance > 4000.0f);
        bool altitudeTooHigh = (myAltitude > 10000.0f);
        // 🔥 altitudeTooLow 조건 제거 - AltitudeSafetyCheck에서 처리

        bool shouldDisengage = (significantSpeedAdvantage && favorableGeometry) || 
                            altitudeTooHigh;

        if (shouldDisengage) {
            std::cout << "[ShouldDisengage] Speed:" << significantSpeedAdvantage 
                    << " Geom:" << favorableGeometry << " AltHigh:" << altitudeTooHigh << std::endl;
        }

        return shouldDisengage;
    }
}
