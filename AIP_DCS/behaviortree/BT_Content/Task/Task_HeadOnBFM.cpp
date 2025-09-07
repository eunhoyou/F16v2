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
        Optional<CPPBlackBoard*> BB_opt = getInput<CPPBlackBoard*>("BB");
        if (!BB_opt || !(*BB_opt)) {
            return NodeStatus::FAILURE;
        }
        CPPBlackBoard* BB = BB_opt.value();
        
        float distance = BB->Distance;
        float los = BB->Los_Degree;
        float aspectAngle = BB->MyAspectAngle_Degree;
        float angleOff = BB->MyAngleOff_Degree;

        std::cout << "[Task_HeadOnBFM] Distance: " << distance 
                  << "m, LOS: " << los << ", AA: " << aspectAngle 
                  << ", AO: " << angleOff << std::endl;

        // 교본: "에스케이프 윈도우 상태를 항상 파악"
        if (ShouldDisengage(BB)) {
            std::cout << "[Task_HeadOnBFM] Escape window open - disengaging" << std::endl;
            BB->VP_Cartesian = CalculateDisengagement(BB);
            BB->Throttle = 1.0f;
            return NodeStatus::SUCCESS;
        }

        // 교본: WEZ 우선 체크 (시뮬레이터 특성)
        if (IsInWEZ(distance, los)) {
            std::cout << "[Task_HeadOnBFM] In WEZ - immediate engagement" << std::endl;
            BB->VP_Cartesian = BB->TargetLocaion_Cartesian; // 단순 조준
            BB->Throttle = 0.8f;
            return NodeStatus::SUCCESS;
        }

        // 교본 기반 거리별 전술 선택
        if (distance > LONG_RANGE_THRESHOLD) {
            // 원거리: 전술적 접근
            std::cout << "[Task_HeadOnBFM] Long range - tactical approach" << std::endl;
            BB->VP_Cartesian = CalculateTacticalApproach(BB);
            BB->Throttle = 0.9f;
        }
        else if (distance > MEDIUM_RANGE_THRESHOLD) {
            // 중거리: 리드 턴 고려
            if (ShouldInitiateLeadTurn(BB)) {
                std::cout << "[Task_HeadOnBFM] Medium range - lead turn" << std::endl;
                BB->VP_Cartesian = CalculateLeadTurn(BB);
                BB->Throttle = CalculateCornerSpeedThrottle(BB);
            } else {
                std::cout << "[Task_HeadOnBFM] Medium range - offset approach" << std::endl;
                BB->VP_Cartesian = CalculateOffsetApproach(BB);
                BB->Throttle = 0.85f;
            }
        }
        else if (distance > CLOSE_RANGE_THRESHOLD) {
            // 근거리: 에너지 상태 기반 기동 선택
            float energyState = CalculateEnergyState(BB);
            
            if (energyState > ENERGY_ADVANTAGE_THRESHOLD) {
                std::cout << "[Task_HeadOnBFM] Energy advantage - aggressive slice" << std::endl;
                BB->VP_Cartesian = CalculateAggressiveSlice(BB);
                BB->Throttle = 0.7f;
            } else if (energyState < -ENERGY_ADVANTAGE_THRESHOLD) {
                std::cout << "[Task_HeadOnBFM] Energy disadvantage - vertical maneuver" << std::endl;
                BB->VP_Cartesian = CalculateVerticalManeuver(BB);
                BB->Throttle = 1.0f;
            } else {
                std::cout << "[Task_HeadOnBFM] Neutral energy - standard slice" << std::endl;
                BB->VP_Cartesian = CalculateStandardSlice(BB);
                BB->Throttle = CalculateCornerSpeedThrottle(BB);
            }
        }
        else {
            // 초근거리: BFM 전환 준비
            std::cout << "[Task_HeadOnBFM] Very close range - BFM transition" << std::endl;
            BB->VP_Cartesian = CalculateBFMTransition(BB);
            BB->Throttle = CalculateCornerSpeedThrottle(BB);
        }

        return NodeStatus::SUCCESS;
    }

    Vector3 Task_HeadOnBFM::CalculateTacticalApproach(CPPBlackBoard* BB) {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        Vector3 targetRight = BB->TargetRightVector;
        float distance = BB->Distance;

        // 교본: "적기의 턴 서클 밖에서는 터닝 룸을 만들려고 하면 안된다"
        // 제한된 오프셋으로 안전한 접근
        float offsetAngle = 15.0f * M_PI / 180.0f; // 15도로 보수적 설정
        float approachDistance = std::min(distance * 0.5f, 1500.0f);

        // 측면 선택: 에너지와 지형 고려
        Vector3 toMe = myLocation - targetLocation;
        float rightDot = toMe.dot(targetRight);
        float sideMultiplier = (rightDot > 0) ? 1.0f : -1.0f;

        Vector3 sideOffset = targetRight * sideMultiplier * approachDistance * std::sin(offsetAngle);
        Vector3 frontOffset = targetForward * (-approachDistance * std::cos(offsetAngle));
        Vector3 tacticalPoint = targetLocation + frontOffset + sideOffset;

        // 고도 관리: 교본의 "위치 에너지" 개념
        float myAltitude = std::abs(myLocation.Z);
        float targetAltitude = std::abs(targetLocation.Z);
        
        if (myAltitude < targetAltitude + 200.0f) {
            tacticalPoint.Z = myLocation.Z + 300.0f; // 300m 상승으로 고도 우위
        } else {
            tacticalPoint.Z = myLocation.Z; // 현재 고도 유지
        }

        return tacticalPoint;
    }

    Vector3 Task_HeadOnBFM::CalculateLeadTurn(CPPBlackBoard* BB)
    {
        // 교본: "리드 턴은 에너지를 위치로 바꾸는 가장 효과적인 방법"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;
        float distance = BB->Distance;

        // 교본: "적기의 각속도가 빠르게 커지기 시작할 때 선회 시작"
        Vector3 toTarget = (targetLocation - myLocation) / distance;
        float dotRight = toTarget.dot(myRight);
        Vector3 leadDirection = (dotRight > 0) ? myRight : myRight * -1.0f;

        // 교본: "최대 G로 리드 턴" - 코너 속도에서 8G
        float cornerSpeed = CalculateCornerSpeed(BB);
        float turnRadius = CalculateTurnRadius(cornerSpeed, 8.0f);
        float leadDistance = turnRadius * 1.8f; // 교본의 리드 턴 비율

        Vector3 leadTurnPoint = myLocation + leadDirection * leadDistance + myForward * (mySpeed * 2.0f);
        
        // 교본: "기수를 10도 정도 아래로 한 슬라이스 턴"
        leadTurnPoint.Z = myLocation.Z - 150.0f; // NED에서 150m 강하

        std::cout << "[CalculateLeadTurn] Turn radius: " << turnRadius 
                  << "m, Lead distance: " << leadDistance << "m" << std::endl;

        return leadTurnPoint;
    }

    Vector3 Task_HeadOnBFM::CalculateAggressiveSlice(CPPBlackBoard* BB)
    {
        // 교본: "에너지 우위 시 공격적 슬라이스 턴"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;
        float distance = BB->Distance;

        Vector3 toTarget = (targetLocation - myLocation) / distance;
        float dotRight = toTarget.dot(myRight);
        Vector3 sliceDirection = (dotRight > 0) ? myRight : myRight * -1.0f;

        // 교본: "8G로 슬라이스" - 공격적 기동
        float turnRadius = CalculateTurnRadius(mySpeed, 8.0f);
        float sliceDistance = turnRadius * 2.0f;

        Vector3 slicePoint = myLocation + sliceDirection * sliceDistance + myForward * (mySpeed * 2.5f);
        
        // 교본: "기수를 수평선 아래로 하고 슬라이스" - 래디얼 G 이득
        slicePoint.Z = myLocation.Z - 400.0f; // NED에서 400m 강하

        std::cout << "[AggressiveSlice] 8G turn, radius: " << turnRadius << "m" << std::endl;
        return slicePoint;
    }

    Vector3 Task_HeadOnBFM::CalculateStandardSlice(CPPBlackBoard* BB)
    {
        // 교본: "표준 슬라이스 - 7G 선회"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;
        float distance = BB->Distance;

        Vector3 toTarget = (targetLocation - myLocation) / distance;
        float dotRight = toTarget.dot(myRight);
        Vector3 turnDirection = (dotRight > 0) ? myRight : myRight * -1.0f;

        float turnRadius = CalculateTurnRadius(mySpeed, 7.0f);
        float turnDistance = turnRadius * 1.5f;

        Vector3 slicePoint = myLocation + turnDirection * turnDistance + myForward * (mySpeed * 2.0f);
        slicePoint.Z = myLocation.Z - 200.0f; // 적당한 강하

        return slicePoint;
    }

    Vector3 Task_HeadOnBFM::CalculateVerticalManeuver(CPPBlackBoard* BB)
    {
        // 교본: "에너지 열세 시 수직 기동으로 상황 개선"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;
        float myAltitude = std::abs(myLocation.Z);

        // 교본: "오버 더 탑 스피드 확인" - F-16은 250노트
        float overTopSpeed = 70.0f; // 250노트 ≈ 70m/s
        
        if (mySpeed > overTopSpeed && myAltitude < 8000.0f) {
            // 수직 상승 가능
            Vector3 verticalPoint = myLocation + myForward * (mySpeed * 1.5f);
            verticalPoint.Z = myLocation.Z + 600.0f; // 600m 상승
            std::cout << "[VerticalManeuver] Climbing 600m for energy advantage" << std::endl;
            return verticalPoint;
        } else {
            // 에너지 부족 - 수평 기동으로 에너지 보존
            Vector3 conservePoint = myLocation + myForward * (mySpeed * 3.0f);
            conservePoint.Z = myLocation.Z;
            std::cout << "[VerticalManeuver] Energy conservation - level flight" << std::endl;
            return conservePoint;
        }
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

    Vector3 Task_HeadOnBFM::CalculateBFMTransition(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;

        // BFM 전환을 위한 안정적 위치
        Vector3 transitionPoint = myLocation + myForward * (mySpeed * 1.0f);
        transitionPoint.Z = myLocation.Z;

        return transitionPoint;
    }

    Vector3 Task_HeadOnBFM::CalculateDisengagement(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;

        // 교본: "에스케이프 윈도우가 열려있을 때 신속 이탈"
        float escapeDistance = mySpeed * 8.0f; // 8초간 최대 속도
        Vector3 escapePoint = myLocation + myForward * escapeDistance;
        escapePoint.Z = myLocation.Z + 500.0f; // 상승하며 이탈
        
        std::cout << "[CalculateDisengagement] High speed escape with climb" << std::endl;
        return escapePoint;
    }

    float Task_HeadOnBFM::CalculateEnergyState(CPPBlackBoard* BB)
    {
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float myAltitude = std::abs(BB->MyLocation_Cartesian.Z);
        float targetAltitude = std::abs(BB->TargetLocaion_Cartesian.Z);

        // 교본: "운동 에너지 + 위치 에너지"
        float myEnergy = 0.5f * mySpeed * mySpeed + 9.81f * myAltitude;
        float targetEnergy = 0.5f * targetSpeed * targetSpeed + 9.81f * targetAltitude;
        float avgEnergy = (myEnergy + targetEnergy) * 0.5f;

        if (avgEnergy < 1.0f) return 0.0f;
        return (myEnergy - targetEnergy) / avgEnergy;
    }

    float Task_HeadOnBFM::CalculateCornerSpeed(CPPBlackBoard* BB)
    {
        // 교본: F-16 코너 속도 약 450KCAS ≈ 130m/s
        float altitude = std::abs(BB->MyLocation_Cartesian.Z);
        float baseCornerSpeed = 130.0f;
        
        // 고도 보정
        float altitudeBonus = (altitude / 10000.0f) * 20.0f;
        
        return baseCornerSpeed + altitudeBonus;
    }

    float Task_HeadOnBFM::CalculateTurnRadius(float speed, float gLoad)
    {
        // 교본 공식: TR = V²/(g*G)
        return (speed * speed) / (9.81f * gLoad);
    }

    float Task_HeadOnBFM::CalculateCornerSpeedThrottle(CPPBlackBoard* BB)
    {
        float mySpeed = BB->MySpeed_MS;
        float cornerSpeed = CalculateCornerSpeed(BB);
        
        // 교본: "코너 속도를 유지하도록 노력한다"
        if (mySpeed < cornerSpeed - 20.0f) {
            return 1.0f; // 급가속
        } else if (mySpeed > cornerSpeed + 20.0f) {
            return 0.6f; // 감속
        }
        return 0.8f; // 유지
    }

    bool Task_HeadOnBFM::ShouldInitiateLeadTurn(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;
        float aspectAngle = BB->MyAspectAngle_Degree;
        float angleOff = BB->MyAngleOff_Degree;

        // 교본: "적기의 각속도가 빠르게 커지기 시작할 때"
        bool distanceCheck = (distance > 2000.0f && distance < 4000.0f);
        bool angleCheck = (std::abs(los) < 45.0f);
        bool headOnCheck = (aspectAngle > 120.0f || aspectAngle < 60.0f);
        bool converging = (angleOff > 120.0f);

        return distanceCheck && angleCheck && headOnCheck && converging;
    }

    bool Task_HeadOnBFM::ShouldDisengage(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float angleOff = BB->MyAngleOff_Degree;

        // 교본: "에스케이프 윈도우 상태 판단"
        bool significantSpeedAdvantage = (mySpeed - targetSpeed > 100.0f);
        bool favorableGeometry = (angleOff > 150.0f && distance > 3000.0f);
        bool highAltitudeEscape = (std::abs(BB->MyLocation_Cartesian.Z) > 12000.0f);

        return (significantSpeedAdvantage && favorableGeometry) || highAltitudeEscape;
    }

    bool Task_HeadOnBFM::IsInWEZ(float distance, float los)
    {
        return (distance >= WEZ_MIN_RANGE && distance <= WEZ_MAX_RANGE) && 
               (std::abs(los) <= WEZ_MAX_ANGLE);
    }
}