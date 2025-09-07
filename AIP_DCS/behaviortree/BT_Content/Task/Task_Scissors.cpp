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
        float mySpeed = (*BB)->MySpeed_MS;
        float targetSpeed = (*BB)->TargetSpeed_MS;
        float myAltitude = static_cast<float>(std::abs((*BB)->MyLocation_Cartesian.Z));

        std::cout << "[Task_Scissors] Distance: " << distance 
                  << ", AspectAngle: " << aspectAngle 
                  << ", MySpeed: " << mySpeed 
                  << ", TargetSpeed: " << targetSpeed << std::endl;

        // 에너지 상태 분석
        float energyAdvantage = CalculateEnergyAdvantage(BB.value());
        
        // 시저스 단계 판단
        ScissorsPhase currentPhase = DetermineScissorsPhase(BB.value());
        
        Vector3 calculated_vp;
        float optimalThrottle;

        switch (currentPhase)
        {
            case SCISSORS_ENTRY:
                // 시저스 진입: 에너지 보존하며 적기와 교차 준비
                calculated_vp = CalculateScissorsEntry(BB.value());
                optimalThrottle = CalculateEnergyConservingThrottle(mySpeed, myAltitude);
                std::cout << "[Task_Scissors] Phase: ENTRY" << std::endl;
                break;

            case SCISSORS_CROSSING:
                // 교차 중: 적기의 에너지 상태에 따른 기동 선택
                if (energyAdvantage > 0.1f)
                {
                    // 에너지 우위: 적극적 시저스
                    calculated_vp = CalculateAggressiveScissors(BB.value());
                    optimalThrottle = 0.9f;
                    std::cout << "[Task_Scissors] Phase: CROSSING (Aggressive)" << std::endl;
                }
                else if (energyAdvantage < -0.1f)
                {
                    // 에너지 열세: 방어적 시저스
                    calculated_vp = CalculateDefensiveScissors(BB.value());
                    optimalThrottle = 0.7f;
                    std::cout << "[Task_Scissors] Phase: CROSSING (Defensive)" << std::endl;
                }
                else
                {
                    // 균등한 에너지: 표준 시저스
                    calculated_vp = CalculateStandardScissors(BB.value());
                    optimalThrottle = 0.8f;
                    std::cout << "[Task_Scissors] Phase: CROSSING (Standard)" << std::endl;
                }
                break;

            case SCISSORS_REPOSITIONING:
                // 재위치: 다음 교차를 위한 최적 위치 확보
                calculated_vp = CalculateRepositioning(BB.value());
                optimalThrottle = CalculateRepositioningThrottle(mySpeed, targetSpeed, energyAdvantage);
                std::cout << "[Task_Scissors] Phase: REPOSITIONING" << std::endl;
                break;

            case SCISSORS_ESCAPE_ATTEMPT:
                // 이탈 시도: 에너지 우위를 바탕으로 시저스에서 벗어나기
                calculated_vp = CalculateScissorsEscape(BB.value());
                optimalThrottle = 1.0f;
                std::cout << "[Task_Scissors] Phase: ESCAPE_ATTEMPT" << std::endl;
                break;

            default:
                // 기본: 표준 시저스
                calculated_vp = CalculateStandardScissors(BB.value());
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

        // 이탈 기회 판단 (상당한 에너지 우위가 있을 때)
        if (energyAdvantage > 0.3f && distance > 1000.0f)
        {
            return SCISSORS_ESCAPE_ATTEMPT;
        }

        // 교차 중 판단 (높은 각속도, 가까운 거리)
        if (distance < 800.0f && std::abs(los) > 45.0f && angleOff > 90.0f)
        {
            return SCISSORS_CROSSING;
        }

        // 재위치 단계 (교차 후 다음 기동 준비)
        if (distance > 800.0f && distance < 1500.0f && aspectAngle > 45.0f && aspectAngle < 135.0f)
        {
            return SCISSORS_REPOSITIONING;
        }

        // 기본: 시저스 진입
        return SCISSORS_ENTRY;
    }

    Vector3 Task_Scissors::CalculateScissorsEntry(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        Vector3 myForward = BB->MyForwardVector;
        float distance = BB->Distance;

        // 적기와 교차하기 위한 진입 경로
        Vector3 toTarget = targetLocation - myLocation;
        float dotRight = toTarget.dot(myRight);
        
        // 교차각을 위한 측방 이동
        Vector3 entryDirection = (dotRight > 0) ? myRight : myRight * -1.0f;
        float entryDistance = std::min(distance * 0.3f, 600.0f);

        Vector3 entryPoint = myLocation + entryDirection * entryDistance + myForward * 400.0f;

        return entryPoint;
    }

    Vector3 Task_Scissors::CalculateAggressiveScissors(CPPBlackBoard* BB)
    {
        // 에너지 우위 시: 적극적으로 적기의 6시를 향해 기수를 당김
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        Vector3 myRight = BB->MyRightVector;
        float mySpeed = BB->MySpeed_MS;

        // 적기의 예측 위치 (더 공격적인 리드)
        float leadTime = 2.5f; // 공격적인 리드 타임
        Vector3 predictedTarget = targetLocation + targetForward * BB->TargetSpeed_MS * leadTime;

        Vector3 toPredict = predictedTarget - myLocation;
        Vector3 aggressivePoint = myLocation + toPredict * 1.2f; // 오버슛 감수하고 공격적 접근

        // 약간의 고도 우위 확보
        aggressivePoint.Z = myLocation.Z - 100.0f;

        return aggressivePoint;
    }

    Vector3 Task_Scissors::CalculateDefensiveScissors(CPPBlackBoard* BB)
    {
        // 에너지 열세 시: 보수적으로 에너지 보존하며 방어적 기동
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        Vector3 myForward = BB->MyForwardVector;

        // 적기로부터 안전한 각도 유지
        Vector3 toTarget = targetLocation - myLocation;
        Vector3 defensiveDirection = myRight;
        
        if (toTarget.dot(myRight) > 0)
        {
            defensiveDirection = myRight * -1.0f;
        }

        float defensiveDistance = 500.0f;
        Vector3 defensivePoint = myLocation + defensiveDirection * defensiveDistance;
        
        // 에너지 보존을 위한 수평 기동
        defensivePoint.Z = myLocation.Z;

        return defensivePoint;
    }

    Vector3 Task_Scissors::CalculateStandardScissors(CPPBlackBoard* BB)
    {
        // 균등한 에너지 상태: 표준적인 시저스 기동
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        Vector3 myForward = BB->MyForwardVector;
        float distance = BB->Distance;

        Vector3 toTarget = targetLocation - myLocation;
        float dotRight = toTarget.dot(myRight);
        
        // 적기를 향한 중간 정도의 선회
        Vector3 turnDirection = (dotRight > 0) ? myRight : myRight * -1.0f;
        float turnDistance = distance * 0.6f;

        Vector3 scissorsPoint = myLocation + turnDirection * turnDistance;
        scissorsPoint.Z = myLocation.Z; // 수평 유지

        return scissorsPoint;
    }

    Vector3 Task_Scissors::CalculateRepositioning(CPPBlackBoard* BB)
    {
        // 다음 교차를 위한 재위치
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;

        // 에너지 회복을 위한 직선 비행 구간
        float repositionDistance = mySpeed * 3.0f; // 3초간 직선 비행
        Vector3 repositionPoint = myLocation + myForward * repositionDistance;

        // 약간의 고도 상승으로 에너지 저장
        repositionPoint.Z = myLocation.Z - 150.0f;  // 150m 상승

        return repositionPoint;
    }

    Vector3 Task_Scissors::CalculateScissorsEscape(CPPBlackBoard* BB)
    {
        // 시저스에서 이탈 시도
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;

        // 에너지 우위를 활용한 이탈 기동
        float escapeDistance = mySpeed * 5.0f; // 5초간 최대 속도 이탈
        Vector3 escapePoint = myLocation + myForward * escapeDistance;

        // 상승하면서 이탈 (위치 에너지로 변환)
        escapePoint.Z = myLocation.Z - 300.0f;  // 300m 상승

        return escapePoint;
    }

    float Task_Scissors::CalculateEnergyAdvantage(CPPBlackBoard* BB)
    {
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float myAltitude = std::abs(BB->MyLocation_Cartesian.Z);
        float targetAltitude = std::abs(BB->TargetLocaion_Cartesian.Z);

        // 운동 에너지 비교
        float myKineticEnergy = 0.5f * mySpeed * mySpeed;
        float targetKineticEnergy = 0.5f * targetSpeed * targetSpeed;

        // 위치 에너지 비교 (고도)
        float myPotentialEnergy = 9.81f * myAltitude;
        float targetPotentialEnergy = 9.81f * targetAltitude;

        // 총 에너지 비교
        float myTotalEnergy = myKineticEnergy + myPotentialEnergy;
        float targetTotalEnergy = targetKineticEnergy + targetPotentialEnergy;

        // 정규화된 에너지 우위 (-1.0 ~ 1.0)
        float energyDifference = myTotalEnergy - targetTotalEnergy;
        float averageEnergy = (myTotalEnergy + targetTotalEnergy) * 0.5f;

        return energyDifference / averageEnergy;
    }

    float Task_Scissors::CalculateEnergyConservingThrottle(float mySpeed, float altitude)
    {
        // 에너지 보존을 위한 스로틀 계산
        float optimalSpeed = 140.0f + (altitude / 10000.0f) * 20.0f;

        if (mySpeed < optimalSpeed - 10.0f)
        {
            return 0.9f; // 속도 부족시 가속
        }
        else if (mySpeed > optimalSpeed + 10.0f)
        {
            return 0.6f; // 속도 과다시 감속
        }
        else
        {
            return 0.75f; // 적정 속도 유지
        }
    }

    float Task_Scissors::CalculateRepositioningThrottle(float mySpeed, float targetSpeed, float energyAdvantage)
    {
        // 재위치 단계에서의 스로틀 조절
        if (energyAdvantage > 0.2f)
        {
            return 0.7f; // 에너지 우위시 보존
        }
        else if (energyAdvantage < -0.2f)
        {
            return 0.95f; // 에너지 열세시 회복
        }
        else
        {
            return 0.8f; // 균등시 표준
        }
    }
}