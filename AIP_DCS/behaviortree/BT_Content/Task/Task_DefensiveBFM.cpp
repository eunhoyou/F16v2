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
        float mySpeed = (*BB)->MySpeed_MS;

        std::cout << "[Task_DefensiveBFM] Distance: " << distance 
                  << "m, LOS: " << los << "°, Speed: " << mySpeed << "m/s" << std::endl;

        // WEZ 위험도에 따른 방어 전략 선택
        if (IsInWEZ(distance, los))
        {
            // WEZ 내 - 긴급 회피
            std::cout << "[Task_DefensiveBFM] IN WEZ! Emergency evasion!" << std::endl;
            (*BB)->VP_Cartesian = CalculateEmergencyEvasion(BB.value());
            (*BB)->Throttle = 1.0f;  // 최대 추력
        }
        else if (IsApproachingWEZ(distance, los))
        {
            // WEZ 접근 - 적극적 방어
            std::cout << "[Task_DefensiveBFM] Approaching WEZ! Aggressive defensive turn!" << std::endl;
            (*BB)->VP_Cartesian = CalculateAggressiveDefense(BB.value());
            (*BB)->Throttle = 1.0f;  // 최대 추력
        }
        else
        {
            // 일반적인 방어 상황 - 표준 방어 선회
            std::cout << "[Task_DefensiveBFM] Standard defensive turn!" << std::endl;
            (*BB)->VP_Cartesian = CalculateDefensiveTurn(BB.value());
            (*BB)->Throttle = CalculateDefensiveThrottle(mySpeed);
        }

        return NodeStatus::SUCCESS;
    }

    Vector3 Task_DefensiveBFM::CalculateEmergencyEvasion(CPPBlackBoard* BB)
    {
        // WEZ 내에서 즉시 벗어나기 위한 최우선 기동
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        Vector3 myForward = BB->MyForwardVector;
        
        // WEZ에서 가장 빠르게 벗어나는 방향 계산
        Vector3 toTarget = targetLocation - myLocation;
        Vector3 escapeDirection;
        
        // 적기와 수직 방향으로 즉시 회피 (90도 방향)
        if (toTarget.dot(myRight) > 0)
        {
            escapeDirection = myRight * -1.0f;  // 왼쪽으로 회피
        }
        else
        {
            escapeDirection = myRight;  // 오른쪽으로 회피
        }
        
        // WEZ 최대 거리를 벗어날 만큼 충분한 거리로 회피
        float escapeDistance = WEZ_MAX_RANGE * 1.5f + BB->MySpeed_MS * 3.0f;
        Vector3 escapePoint = myLocation + escapeDirection * escapeDistance;
        
        // 약간의 고도 변경으로 3차원 기동 (교본: 수직 공간 활용)
        escapePoint.Z = myLocation.Z - 150.0f;  // 150m 상승
        
        std::cout << "[EmergencyEvasion] Escape distance: " << escapeDistance << "m" << std::endl;
        return escapePoint;
    }

    Vector3 Task_DefensiveBFM::CalculateAggressiveDefense(CPPBlackBoard* BB)
    {
        // WEZ 접근 시 적기의 BFM 문제를 유발하는 적극적 방어
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        float mySpeed = BB->MySpeed_MS;
        float distance = BB->Distance;
        
        // 적기 쪽으로 공격적인 방어 선회 (BFM 문제 유발)
        Vector3 toTarget = targetLocation - myLocation;
        Vector3 turnDirection;
        
        // 적기 쪽으로 선회하여 양력벡터를 적기에게 놓기
        if (toTarget.dot(myRight) > 0)
        {
            turnDirection = myRight;
        }
        else
        {
            turnDirection = myRight * -1.0f;
        }
        
        // 코너 속도 기준 최대 선회율로 기동
        float cornerSpeed = CalculateCornerSpeed(BB);
        float turnRadius = CalculateTurnRadius(cornerSpeed, 8.0f);  // 8G 공격적 선회
        float aggressiveDistance = turnRadius * 2.0f;
        
        Vector3 defensePoint = myLocation + turnDirection * aggressiveDistance;
        
        // 교본: "래디얼 G를 증가시키기 위한" 약간의 강하
        defensePoint.Z = myLocation.Z + 100.0f;  // 100m 강하 (NED)
        
        std::cout << "[AggressiveDefense] Turn radius: " << turnRadius 
                  << "m, Aggressive distance: " << aggressiveDistance << "m" << std::endl;
        
        return defensePoint;
    }

    Vector3 Task_DefensiveBFM::CalculateDefensiveTurn(CPPBlackBoard* BB)
    {
        // 교본: "양력벡터를 적기에게 놓고 코너 속도에서 최대 G로 선회"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        float mySpeed = BB->MySpeed_MS;
        
        // 적기 방향으로 선회 방향 결정
        Vector3 toTarget = targetLocation - myLocation;
        Vector3 turnDirection;
        
        if (toTarget.dot(myRight) > 0)
        {
            turnDirection = myRight;
        }
        else
        {
            turnDirection = myRight * -1.0f;
        }
        
        // 표준 방어 선회 (7G)
        float cornerSpeed = CalculateCornerSpeed(BB);
        float turnRadius = CalculateTurnRadius(cornerSpeed, 7.0f);
        float defensiveDistance = turnRadius * 1.5f;
        
        Vector3 defensivePoint = myLocation + turnDirection * defensiveDistance;
        
        // 수평 유지 (일반적인 방어 상황)
        defensivePoint.Z = myLocation.Z;
        
        std::cout << "[DefensiveTurn] Standard turn radius: " << turnRadius 
                  << "m, Corner speed: " << cornerSpeed << "m/s" << std::endl;
        
        return defensivePoint;
    }

    bool Task_DefensiveBFM::IsInWEZ(float distance, float los)
    {
        // WEZ 조건: 152.4-914.4m, ±2도
        bool rangeCheck = (distance >= WEZ_MIN_RANGE && distance <= WEZ_MAX_RANGE);
        bool angleCheck = (std::abs(los) <= WEZ_MAX_ANGLE);
        
        return rangeCheck && angleCheck;
    }

    bool Task_DefensiveBFM::IsApproachingWEZ(float distance, float los)
    {
        // WEZ 접근 상황: WEZ보다 약간 넓은 범위
        float approachRange = WEZ_MAX_RANGE * 1.3f;  // WEZ 최대 거리의 1.3배
        float approachAngle = WEZ_MAX_ANGLE * 2.5f;  // WEZ 각도의 2.5배
        
        bool rangeCheck = (distance >= WEZ_MIN_RANGE && distance <= approachRange);
        bool angleCheck = (std::abs(los) <= approachAngle);
        
        return rangeCheck && angleCheck;
    }

    float Task_DefensiveBFM::CalculateCornerSpeed(CPPBlackBoard* BB)
    {
        // F-16 코너 속도: 약 450KCAS (교본 기준)
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
        if (mySpeed < cornerSpeed - 15.0f)
        {
            return 1.0f;  // 최대 추력으로 가속
        }
        else if (mySpeed > cornerSpeed + 15.0f)
        {
            return 0.7f;  // 추력 감소 (선회 성능 우선)
        }
        else
        {
            return 0.9f;  // 적당한 추력 유지
        }
    }
}