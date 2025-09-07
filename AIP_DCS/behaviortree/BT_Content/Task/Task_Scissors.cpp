#include "Task_Scissors.h"

namespace Action
{
    PortsList Task_Scissors::providedPorts()
    {
        return {
            InputPort<CPPBlackBoard*>("BB")
        };
    }

    NodeStatus Task_Scissors::tick()
    {
        Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");

        float distance = (*BB)->Distance;
        float aspectAngle = (*BB)->MyAspectAngle_Degree;
        float angleOff = (*BB)->MyAngleOff_Degree;
        float los = (*BB)->Los_Degree;
        float mySpeed = (*BB)->MySpeed_MS;
        float targetSpeed = (*BB)->TargetSpeed_MS;

        std::cout << "[Task_Scissors] Distance: " << distance 
                  << ", AA: " << aspectAngle 
                  << ", AO: " << angleOff
                  << ", Speed: " << mySpeed << " vs " << targetSpeed << std::endl;

        // 교본: WEZ 내부면 즉시 교전 시도
        if (IsInWEZ(distance, los))
        {
            std::cout << "[Task_Scissors] In WEZ during scissors - immediate engagement!" << std::endl;
            (*BB)->VP_Cartesian = (*BB)->TargetLocaion_Cartesian;
            (*BB)->Throttle = 0.8f;
            return NodeStatus::SUCCESS;
        }

        // 교본: "Rate Kills" - 에너지 상태 분석이 시저스의 핵심
        float energyAdvantage = CalculateEnergyAdvantage(BB.value());
        ScissorsPhase currentPhase = DetermineScissorsPhase(BB.value());
        
        Vector3 calculated_vp;
        float optimalThrottle;

        switch (currentPhase)
        {
            case SCISSORS_NEUTRAL_SETUP:
                // 교본: "두 비행기가 옆으로 나란한 중립 위치"
                calculated_vp = CalculateNeutralSetup(BB.value());
                optimalThrottle = CalculateEnergyConservingThrottle(mySpeed);
                std::cout << "[Task_Scissors] Phase: NEUTRAL_SETUP" << std::endl;
                break;

            case SCISSORS_ENERGY_FIGHT:
                // 교본: "선회를 더 잘하는 비행기가 이긴다"
                if (energyAdvantage > ENERGY_ADVANTAGE_THRESHOLD)
                {
                    calculated_vp = CalculateAggressiveScissors(BB.value());
                    optimalThrottle = 0.9f;
                    std::cout << "[Task_Scissors] Phase: ENERGY_FIGHT (Aggressive)" << std::endl;
                }
                else if (energyAdvantage < -ENERGY_ADVANTAGE_THRESHOLD)
                {
                    calculated_vp = CalculateDefensiveScissors(BB.value());
                    optimalThrottle = 0.6f; // 에너지 보존
                    std::cout << "[Task_Scissors] Phase: ENERGY_FIGHT (Defensive)" << std::endl;
                }
                else
                {
                    calculated_vp = CalculateNeutralScissors(BB.value());
                    optimalThrottle = 0.75f;
                    std::cout << "[Task_Scissors] Phase: ENERGY_FIGHT (Neutral)" << std::endl;
                }
                break;

            case SCISSORS_RATE_FIGHT:
                // 교본: "더 높은 선회율을 가진 전투기가 유리"
                calculated_vp = CalculateRateFight(BB.value());
                optimalThrottle = CalculateCornerSpeedThrottle(BB.value());
                std::cout << "[Task_Scissors] Phase: RATE_FIGHT" << std::endl;
                break;

            case SCISSORS_ESCAPE_OPPORTUNITY:
                // 교본: "에너지 우위를 바탕으로 시저스에서 벗어나기"
                calculated_vp = CalculateScissorsEscape(BB.value());
                optimalThrottle = 1.0f;
                std::cout << "[Task_Scissors] Phase: ESCAPE_OPPORTUNITY" << std::endl;
                break;

            case SCISSORS_EMERGENCY_SEPARATION:
                // 위급 상황 - 충돌 방지 및 안전 이격
                calculated_vp = CalculateEmergencySeparation(BB.value());
                optimalThrottle = 1.0f;
                std::cout << "[Task_Scissors] Phase: EMERGENCY_SEPARATION" << std::endl;
                break;

            default:
                calculated_vp = CalculateNeutralScissors(BB.value());
                optimalThrottle = 0.8f;
                break;
        }

        (*BB)->VP_Cartesian = calculated_vp;
        (*BB)->Throttle = optimalThrottle;

        std::cout << "[Task_Scissors] Energy advantage: " << energyAdvantage 
                  << ", Throttle: " << optimalThrottle << std::endl;

        return NodeStatus::SUCCESS;
    }

    Task_Scissors::ScissorsPhase Task_Scissors::DetermineScissorsPhase(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float aspectAngle = BB->MyAspectAngle_Degree;
        float angleOff = BB->MyAngleOff_Degree;
        float los = BB->Los_Degree;
        float energyAdvantage = CalculateEnergyAdvantage(BB);

        // 교본: "에스케이프 윈도우 판단"
        if (ShouldEscapeScissors(BB))
        {
            return SCISSORS_ESCAPE_OPPORTUNITY;
        }

        // 응급 상황: 충돌 위험
        if (distance < COLLISION_AVOIDANCE_DISTANCE)
        {
            return SCISSORS_EMERGENCY_SEPARATION;
        }

        // 교본: "Rate Fight" - 가까운 거리에서 높은 각속도
        if (distance < RATE_FIGHT_DISTANCE && std::abs(los) > 30.0f && angleOff > 90.0f)
        {
            return SCISSORS_RATE_FIGHT;
        }

        // 교본: "에너지 싸움"
        if (distance >= RATE_FIGHT_DISTANCE && distance < ENERGY_FIGHT_DISTANCE)
        {
            return SCISSORS_ENERGY_FIGHT;
        }

        // 기본: 중립 셋업
        return SCISSORS_NEUTRAL_SETUP;
    }

    Vector3 Task_Scissors::CalculateNeutralSetup(CPPBlackBoard* BB)
    {
        // 교본: "두 비행기가 옆으로 나란한 중립 위치에서 시저스 시작"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        Vector3 myForward = BB->MyForwardVector;

        // 적기와 나란한 위치로 이동
        Vector3 toTarget = targetLocation - myLocation;
        float distance = BB->Distance;
        Vector3 toTargetNorm = toTarget / distance;

        // 측면 위치 계산
        float sideOffset = distance * 0.4f; // 40% 측면 이격
        Vector3 neutralPoint;

        if (toTargetNorm.dot(myRight) > 0)
        {
            neutralPoint = targetLocation - myRight * sideOffset;
        }
        else
        {
            neutralPoint = targetLocation + myRight * sideOffset;
        }

        neutralPoint.Z = myLocation.Z; // 고도 유지
        return neutralPoint;
    }

    Vector3 Task_Scissors::CalculateAggressiveScissors(CPPBlackBoard* BB)
    {
        // 교본: "에너지 우위 시 적극적으로 적기의 6시를 향해 기수를 당김"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        Vector3 myRight = BB->MyRightVector;
        float targetSpeed = BB->TargetSpeed_MS;

        // 교본: "리드 추적으로 적기의 예측 위치 공격"
        float leadTime = 2.0f; // 공격적인 리드
        Vector3 predictedTarget = targetLocation + targetForward * targetSpeed * leadTime;

        Vector3 toPredict = predictedTarget - myLocation;
        float predictDistance = myLocation.distance(predictedTarget);
        if (predictDistance > 0.1f)
        {
            toPredict = toPredict / predictDistance;
        }

        // 교본: "코너 속도에서 최대 G로 선회"
        float cornerSpeed = CalculateCornerSpeed(BB);
        float turnRadius = CalculateTurnRadius(cornerSpeed, 8.0f);
        
        Vector3 aggressivePoint = myLocation + toPredict * (turnRadius * 1.5f);
        aggressivePoint.Z = myLocation.Z - 100.0f; // 약간 상승

        std::cout << "[AggressiveScissors] Lead time: " << leadTime 
                  << "s, Turn radius: " << turnRadius << "m" << std::endl;

        return aggressivePoint;
    }

    Vector3 Task_Scissors::CalculateDefensiveScissors(CPPBlackBoard* BB)
    {
        // 교본: "에너지 열세 시 에너지 보존하며 방어적 기동"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        Vector3 myForward = BB->MyForwardVector;

        // 적기로부터 안전한 각도 유지
        Vector3 toTarget = targetLocation - myLocation;
        Vector3 defensiveDirection;
        
        if (toTarget.dot(myRight) > 0)
        {
            defensiveDirection = myRight * -1.0f;
        }
        else
        {
            defensiveDirection = myRight;
        }

        // 교본: "전방 이동 속도를 상대적으로 빨리 늦추는 비행기가 이긴다"
        float defensiveDistance = 400.0f; // 보수적 거리
        Vector3 defensivePoint = myLocation + defensiveDirection * defensiveDistance;
        
        // 에너지 보존을 위한 수평 기동
        defensivePoint.Z = myLocation.Z;

        return defensivePoint;
    }

    Vector3 Task_Scissors::CalculateNeutralScissors(CPPBlackBoard* BB)
    {
        // 교본: "균등한 에너지 상태에서 표준적인 시저스 기동"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        float distance = BB->Distance;

        Vector3 toTarget = targetLocation - myLocation;
        float dotRight = toTarget.dot(myRight);
        
        // 적기를 향한 중간 정도의 선회
        Vector3 turnDirection = (dotRight > 0) ? myRight : myRight * -1.0f;
        float turnDistance = distance * 0.5f;

        Vector3 scissorsPoint = myLocation + turnDirection * turnDistance;
        scissorsPoint.Z = myLocation.Z; // 수평 유지

        return scissorsPoint;
    }

    Vector3 Task_Scissors::CalculateRateFight(CPPBlackBoard* BB)
    {
        // 교본: "Rate Kills - 선회율이 격추한다"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        float mySpeed = BB->MySpeed_MS;

        // 교본: "코너 속도에서 최대 선회율"
        float cornerSpeed = CalculateCornerSpeed(BB);
        Vector3 toTarget = targetLocation - myLocation;
        float distance = BB->Distance;
        Vector3 toTargetNorm = toTarget / distance;

        // 최대 선회율로 적기 쪽으로 기수 돌리기
        float dotRight = toTargetNorm.dot(myRight);
        Vector3 rateDirection = (dotRight > 0) ? myRight : myRight * -1.0f;
        
        float turnRadius = CalculateTurnRadius(cornerSpeed, 9.0f); // 최대 G
        Vector3 ratePoint = myLocation + rateDirection * turnRadius;
        
        // 교본: "수직 기동으로 선회율 증가"
        ratePoint.Z = myLocation.Z + 50.0f; // 약간 강하로 래디얼 G 증가

        std::cout << "[RateFight] Corner speed: " << cornerSpeed 
                  << "m/s, Max turn radius: " << turnRadius << "m" << std::endl;

        return ratePoint;
    }

    Vector3 Task_Scissors::CalculateScissorsEscape(CPPBlackBoard* BB)
    {
        // 교본: "에너지 우위를 활용한 시저스 이탈"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;

        // 수직 기동으로 이탈
        float escapeDistance = mySpeed * 6.0f; // 6초간 최대 속도
        Vector3 escapePoint = myLocation + myForward * escapeDistance;

        // 교본: "위치 에너지로 전환하며 이탈"
        escapePoint.Z = myLocation.Z - 400.0f;  // 400m 상승

        std::cout << "[ScissorsEscape] Vertical escape with " << escapeDistance << "m forward" << std::endl;
        return escapePoint;
    }

    Vector3 Task_Scissors::CalculateEmergencySeparation(CPPBlackBoard* BB)
    {
        // 충돌 방지를 위한 응급 기동
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myUp = BB->MyUpVector;
        Vector3 myRight = BB->MyRightVector;

        // 적기와 반대 방향으로 급격한 분리
        Vector3 toTarget = targetLocation - myLocation;
        Vector3 separationDirection = toTarget * -1.0f; // 적기 반대 방향
        separationDirection = separationDirection / BB->Distance;

        // 수직 성분 추가로 3차원 분리
        Vector3 separationPoint = myLocation + separationDirection * 1000.0f;
        separationPoint.Z = myLocation.Z - 200.0f; // 200m 상승

        std::cout << "[EmergencySeparation] Collision avoidance maneuver!" << std::endl;
        return separationPoint;
    }

    float Task_Scissors::CalculateEnergyAdvantage(CPPBlackBoard* BB)
    {
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float myAltitude = std::abs(BB->MyLocation_Cartesian.Z);
        float targetAltitude = std::abs(BB->TargetLocaion_Cartesian.Z);

        // 교본: "위치를 위한 추력" - 운동 에너지 + 위치 에너지
        float myKineticEnergy = 0.5f * mySpeed * mySpeed;
        float targetKineticEnergy = 0.5f * targetSpeed * targetSpeed;
        float myPotentialEnergy = 9.81f * myAltitude;
        float targetPotentialEnergy = 9.81f * targetAltitude;

        float myTotalEnergy = myKineticEnergy + myPotentialEnergy;
        float targetTotalEnergy = targetKineticEnergy + targetPotentialEnergy;

        // 정규화된 에너지 우위
        float energyDifference = myTotalEnergy - targetTotalEnergy;
        float averageEnergy = (myTotalEnergy + targetTotalEnergy) * 0.5f;

        if (averageEnergy < 1.0f) return 0.0f;
        return energyDifference / averageEnergy;
    }

    float Task_Scissors::CalculateCornerSpeed(CPPBlackBoard* BB)
    {
        // 교본: F-16 코너 속도 약 450KCAS ≈ 130m/s
        float altitude = std::abs(BB->MyLocation_Cartesian.Z);
        float baseCornerSpeed = 130.0f;
        
        // 고도에 따른 보정
        float altitudeBonus = (altitude / 10000.0f) * 20.0f;
        
        return baseCornerSpeed + altitudeBonus;
    }

    float Task_Scissors::CalculateTurnRadius(float speed, float gLoad)
    {
        // 교본 공식: TR = V²/(g*G)
        return (speed * speed) / (9.81f * gLoad);
    }

    float Task_Scissors::CalculateEnergyConservingThrottle(float mySpeed)
    {
        // 교본: "에너지 보존을 위한 스로틀 계산"
        float optimalSpeed = 120.0f; // 에너지 보존 최적 속도

        if (mySpeed < optimalSpeed - 15.0f)
        {
            return 0.9f; // 속도 부족시 가속
        }
        else if (mySpeed > optimalSpeed + 15.0f)
        {
            return 0.5f; // 속도 과다시 감속
        }
        else
        {
            return 0.7f; // 적정 속도 유지
        }
    }

    float Task_Scissors::CalculateCornerSpeedThrottle(CPPBlackBoard* BB)
    {
        float mySpeed = BB->MySpeed_MS;
        float cornerSpeed = CalculateCornerSpeed(BB);
        
        // 교본: "코너 속도를 유지하도록 노력한다"
        if (mySpeed < cornerSpeed - 20.0f)
        {
            return 1.0f; // 급가속
        }
        else if (mySpeed > cornerSpeed + 20.0f)
        {
            return 0.6f; // 감속
        }
        else
        {
            return 0.8f; // 유지
        }
    }

    bool Task_Scissors::ShouldEscapeScissors(CPPBlackBoard* BB)
    {
        float energyAdvantage = CalculateEnergyAdvantage(BB);
        float distance = BB->Distance;
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;

        // 교본: "에스케이프 윈도우가 열려있을 때"
        bool significantEnergyAdvantage = (energyAdvantage > 0.4f);
        bool speedAdvantage = (mySpeed > targetSpeed + 50.0f);
        bool sufficientDistance = (distance > 1000.0f);

        return significantEnergyAdvantage && speedAdvantage && sufficientDistance;
    }

    bool Task_Scissors::IsInWEZ(float distance, float los)
    {
        return (distance >= WEZ_MIN_RANGE && distance <= WEZ_MAX_RANGE) && 
               (std::abs(los) <= WEZ_MAX_ANGLE);
    }