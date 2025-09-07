#include "Task_DefensiveBFM.h"

namespace Action
{
    PortsList Task_DefensiveBFM::providedPorts()
    {
        return {
            InputPort<CPPBlackBoard*>("BB")
        };
    }

    NodeStatus Task_DefensiveBFM::tick()
    {
        Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");

        float distance = (*BB)->Distance;
        float los = (*BB)->Los_Degree;
        float aspectAngle = (*BB)->MyAspectAngle_Degree;
        float mySpeed = (*BB)->MySpeed_MS;

        std::cout << "[Task_DefensiveBFM] Distance: " << distance 
                  << "m, LOS: " << los 
                  << ", AspectAngle: " << aspectAngle 
                  << ", Speed: " << mySpeed << "m/s" << std::endl;

        // 교본 원칙: "가장 가까운 위협에 대항하여 싸우라"
        ThreatLevel currentThreat = AssessThreatLevel(BB.value());
        
        Vector3 calculated_vp;
        float defensive_throttle;

        switch (currentThreat)
        {
            case IMMEDIATE_WEZ_THREAT:
                // WEZ 내 - 교본: "미사일과는 각도로 싸워라"
                std::cout << "[Task_DefensiveBFM] IMMEDIATE WEZ THREAT - Emergency evasion!" << std::endl;
                calculated_vp = CalculateEmergencyWEZEvasion(BB.value());
                defensive_throttle = 1.0f; // 최대 추력
                break;

            case HIGH_THREAT:
                // WEZ 접근 중 - 교본: "양력벡터를 적기에게 놓고 최대 G로 선회"
                std::cout << "[Task_DefensiveBFM] HIGH THREAT - Aggressive defensive turn!" << std::endl;
                calculated_vp = CalculateLiftVectorDefense(BB.value());
                defensive_throttle = 1.0f; // 최대 추력
                break;

            case MODERATE_THREAT:
                // 일반 방어 상황 - 교본: "적기에게 BFM 문제를 유발"
                std::cout << "[Task_DefensiveBFM] MODERATE THREAT - Standard defensive BFM!" << std::endl;
                calculated_vp = CalculateStandardDefensiveTurn(BB.value());
                defensive_throttle = CalculateDefensiveThrottle(mySpeed);
                break;

            case LOW_THREAT:
                // 적기가 BFM 실수 중 - 이용할 기회 모색
                std::cout << "[Task_DefensiveBFM] LOW THREAT - Exploiting enemy mistake!" << std::endl;
                calculated_vp = CalculateCounterAttack(BB.value());
                defensive_throttle = 0.9f;
                break;
        }

        (*BB)->VP_Cartesian = calculated_vp;
        (*BB)->Throttle = defensive_throttle;

        return NodeStatus::SUCCESS;
    }

    Task_DefensiveBFM::ThreatLevel Task_DefensiveBFM::AssessThreatLevel(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;
        float aspectAngle = BB->MyAspectAngle_Degree;
        
        // WEZ 위협 (최우선)
        if (IsInWEZ(distance, los)) {
            return IMMEDIATE_WEZ_THREAT;
        }
        
        // WEZ 접근 위협
        if (IsApproachingWEZ(distance, los)) {
            return HIGH_THREAT;
        }
        
        // 일반적인 방어 상황 판단
        // 교본: "적기가 6시 후방에 있을 때"
        if (aspectAngle < 45.0f && distance < 2000.0f) {
            return MODERATE_THREAT;
        }
        
        // 적기가 실수하고 있는 상황
        // 교본: "적기가 오버슛하거나 에너지를 잃었을 때"
        return LOW_THREAT;
    }

    Vector3 Task_DefensiveBFM::CalculateEmergencyWEZEvasion(CPPBlackBoard* BB)
    {
        // 교본: "미사일과는 각도로 싸워라" - WEZ에서 90도 각도로 즉시 회피
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        
        // WEZ에서 가장 빠르게 벗어나는 방향 (90도 회피)
        Vector3 toTarget = targetLocation - myLocation;
        Vector3 escapeDirection;
        
        // 적기와 수직 방향으로 즉시 회피
        if (toTarget.dot(myRight) > 0) {
            escapeDirection = myRight * -1.0f;  // 왼쪽으로 급회피
        } else {
            escapeDirection = myRight;  // 오른쪽으로 급회피
        }
        
        // WEZ 최대 거리를 확실히 벗어날 거리
        float escapeDistance = WEZ_MAX_RANGE * 2.0f + BB->MySpeed_MS * 2.0f;
        Vector3 escapePoint = myLocation + escapeDirection * escapeDistance;
        
        // 교본: "수직 공간 활용" - 3차원 기동
        escapePoint.Z = myLocation.Z - 200.0f;  // 200m 상승
        
        std::cout << "[EmergencyWEZEvasion] 90-degree escape, distance: " << escapeDistance << "m" << std::endl;
        return escapePoint;
    }

    Vector3 Task_DefensiveBFM::CalculateLiftVectorDefense(CPPBlackBoard* BB)
    {
        // 교본 핵심: "양력벡터를 적기에게 놓고 코너 속도에서 최대 G로 선회"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        float mySpeed = BB->MySpeed_MS;
        float distance = BB->Distance;

        // 교본: "적기 쪽으로 양력벡터를 놓기"
        Vector3 toTarget = (targetLocation - myLocation);
        toTarget = toTarget / distance;  // 정규화, 추후 확인

        // 적기를 향한 선회 방향 결정
        Vector3 turnDirection;
        if (toTarget.dot(myRight) > 0) {
            turnDirection = myRight;
        } else {
            turnDirection = myRight * -1.0f;
        }
        
        // 교본: 코너 속도에서 최대 선회율
        float cornerSpeed = CalculateCornerSpeed(BB);
        float maxGLoad = 8.0f; // 교본: "8G로 브렉 턴"
        float turnRadius = CalculateTurnRadius(cornerSpeed, maxGLoad);
        
        // 양력벡터 방향으로 선회 지점 계산
        float defensiveDistance = turnRadius * 1.5f;
        Vector3 liftVectorPoint = myLocation + turnDirection * defensiveDistance;
        
        // 교본: "기수를 수평선 아래로 하고 돌 때 추가적인 G"
        liftVectorPoint.Z = myLocation.Z + 150.0f;  // 150m 강하 (NED)
        
        std::cout << "[LiftVectorDefense] 8G turn, radius: " << turnRadius 
                  << "m, Corner speed: " << cornerSpeed << "m/s" << std::endl;
        
        return liftVectorPoint;
    }

    Vector3 Task_DefensiveBFM::CalculateStandardDefensiveTurn(CPPBlackBoard* BB)
    {
        // 교본: "적기에게 BFM 문제를 유발하는 표준 방어 선회"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        float mySpeed = BB->MySpeed_MS;
        
        // 적기 방향으로 선회 방향 결정
        Vector3 toTarget = targetLocation - myLocation;
        Vector3 turnDirection;
        
        if (toTarget.dot(myRight) > 0) {
            turnDirection = myRight;
        } else {
            turnDirection = myRight * -1.0f;
        }
        
        // 표준 방어 선회 (7G)
        float cornerSpeed = CalculateCornerSpeed(BB);
        float turnRadius = CalculateTurnRadius(cornerSpeed, 7.0f);
        float defensiveDistance = turnRadius * 1.2f;
        
        Vector3 defensivePoint = myLocation + turnDirection * defensiveDistance;
        
        // 수평 방어 선회 (에너지 보존)
        defensivePoint.Z = myLocation.Z;
        
        std::cout << "[StandardDefensive] 7G turn, radius: " << turnRadius << "m" << std::endl;
        
        return defensivePoint;
    }

    Vector3 Task_DefensiveBFM::CalculateCounterAttack(CPPBlackBoard* BB)
    {
        // 교본: "적기가 실수를 하면 그 실수를 이용한다"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        float aspectAngle = BB->MyAspectAngle_Degree;
        
        // 적기의 예상 움직임을 고려한 반격 위치
        if (aspectAngle > 90.0f) {
            // 적기가 오버슛 중 - 교본: "리버스를 해서 유리한 상황을 얻는다"
            Vector3 reversalPoint = targetLocation + targetForward * 300.0f;
            reversalPoint.Z = myLocation.Z - 100.0f; // 약간 상승
            
            std::cout << "[CounterAttack] Enemy overshoot - Reversal maneuver" << std::endl;
            return reversalPoint;
        } else {
            // 일반적인 반격 기회
            Vector3 counterPoint = targetLocation - targetForward * 200.0f;
            counterPoint.Z = myLocation.Z;
            
            std::cout << "[CounterAttack] Standard counter-attack positioning" << std::endl;
            return counterPoint;
        }
    }

    bool Task_DefensiveBFM::IsInWEZ(float distance, float los)
    {
        bool rangeCheck = (distance >= WEZ_MIN_RANGE && distance <= WEZ_MAX_RANGE);
        bool angleCheck = (std::abs(los) <= WEZ_MAX_ANGLE);
        
        return rangeCheck && angleCheck;
    }

    bool Task_DefensiveBFM::IsApproachingWEZ(float distance, float los)
    {
        float approachRange = WEZ_MAX_RANGE * 1.5f;
        float approachAngle = WEZ_MAX_ANGLE * 3.0f;
        
        bool rangeCheck = (distance >= WEZ_MIN_RANGE && distance <= approachRange);
        bool angleCheck = (std::abs(los) <= approachAngle);
        
        return rangeCheck && angleCheck;
    }

    float Task_DefensiveBFM::CalculateCornerSpeed(CPPBlackBoard* BB)
    {
        // 교본: F-16 코너 속도 약 450KCAS
        float altitude = std::abs(BB->MyLocation_Cartesian.Z);
        float baseCornerSpeed = 130.0f;  // 130 m/s ≈ 450 KCAS
        
        // 고도 보정
        float altitudeBonus = (altitude / 10000.0f) * 15.0f;
        
        return baseCornerSpeed + altitudeBonus;
    }

    float Task_DefensiveBFM::CalculateTurnRadius(float speed, float gLoad)
    {
        // 교본 공식: TR = V²/(g*G)
        return (speed * speed) / (9.81f * gLoad);
    }

    float Task_DefensiveBFM::CalculateDefensiveThrottle(float mySpeed)
    {
        float cornerSpeed = 130.0f;  // 기본 코너 속도
        
        // 교본: "코너 속도를 유지하도록 노력한다"
        if (mySpeed < cornerSpeed - 20.0f) {
            return 1.0f;  // 최대 추력으로 가속
        } else if (mySpeed > cornerSpeed + 20.0f) {
            return 0.6f;  // 추력 감소 (선회 성능 우선)
        } else {
            return 0.9f;  // 적당한 추력 유지
        }
    }