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

        // 교본: "Rate Kills" - 에너지 상태 분석이 시저스의 핵심
        float energyAdvantage = CalculateEnergyAdvantage(BB.value());
        ScissorsPhase currentPhase = DetermineScissorsPhase(BB.value());
        
        Vector3 calculated_vp;
        float optimalThrottle;

        switch (currentPhase)
        {
            case SCISSORS_ENERGY_FIGHT:
                // 교본: "선회를 더 잘하는 비행기가 이긴다"
                if (energyAdvantage > 0.25f)
                {
                    calculated_vp = CalculateAggressiveScissors(BB.value());
                    optimalThrottle = 0.8f;
                }
                else if (energyAdvantage < -0.25f)
                {
                    calculated_vp = CalculateDefensiveScissors(BB.value());
                    optimalThrottle = 0.4f;
                }
                else
                {
                    calculated_vp = CalculateNeutralScissors(BB.value());
                    optimalThrottle = 0.6f;
                }
                break;

            case SCISSORS_ESCAPE_OPPORTUNITY:
                // 교본: "에너지 우위를 바탕으로 시저스에서 벗어나기"
                calculated_vp = CalculateScissorsEscape(BB.value());
                optimalThrottle = 0.8f;
                std::cout << "[Task_Scissors] Phase: ESCAPE_OPPORTUNITY" << std::endl;
                break;

            default:
                calculated_vp = CalculateNeutralScissors(BB.value());
                optimalThrottle = 0.5f;
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

        if (ShouldEscapeScissors(BB))
        {
            return SCISSORS_ESCAPE_OPPORTUNITY;
        }

        if (distance < 3000.0f)
        {
            return SCISSORS_ENERGY_FIGHT;
        }

        return SCISSORS_NEUTRAL_SETUP;
    }

    Vector3 Task_Scissors::CalculateAggressiveScissors(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        targetForward.normalize();
        Vector3 myRight = BB->MyRightVector;
        myRight.normalize();
        float targetSpeed = BB->TargetSpeed_MS;
        float mySpeed = BB->MySpeed_MS;

        float leadTime = 2.0f;
        Vector3 predictedTarget = targetLocation + targetForward * targetSpeed * leadTime;

        Vector3 toPredict = predictedTarget - myLocation;
        toPredict.normalize();

        // 교본: "코너 속도에서 최대 G로 선회"
        float cornerSpeed = CalculateCornerSpeed(BB);
        
        float scissorsDistance = mySpeed * 2.0f;
        Vector3 aggressivePoint = myLocation + toPredict * scissorsDistance;
        aggressivePoint.Z = myLocation.Z + scissorsDistance * 0.1f;

        return aggressivePoint;
    }

    Vector3 Task_Scissors::CalculateDefensiveScissors(CPPBlackBoard* BB)
    {
        // 교본: "에너지 열세 시 에너지 보존하며 방어적 기동"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        myRight.normalize();
        Vector3 myForward = BB->MyForwardVector;
        myForward.normalize();
        float mySpeed = BB->MySpeed_MS;

        // 적기로부터 안전한 각도 유지
        Vector3 toTarget = targetLocation - myLocation;
        toTarget.normalize();

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
        float defensiveDistance = mySpeed * 2.0f;
        Vector3 defensivePoint = myLocation + defensiveDirection * defensiveDistance;
        defensivePoint.Z = myLocation.Z;

        return defensivePoint;
    }

    Vector3 Task_Scissors::CalculateNeutralScissors(CPPBlackBoard* BB)
    {
        // 교본: "균등한 에너지 상태에서 표준적인 시저스 기동"
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        myRight.normalize();
        float distance = BB->Distance;
        float los = BB->Los_Degree;
        float mySpeed = BB->MySpeed_MS;

        Vector3 toTarget = targetLocation - myLocation;
        toTarget.normalize();

        Vector3 scissorsDirection;
        // LOS가 높으면 이미 교차 중 - 반대 방향으로 선회
        if (std::abs(los) > 15.0f) {
            scissorsDirection = (toTarget.dot(myRight) > 0) ? 
                            myRight * -1.0f : myRight;
        } else {
            // 교차 전 - 적기 쪽으로 접근
            scissorsDirection = (toTarget.dot(myRight) > 0) ? 
                            myRight : myRight * -1.0f;
        }

        float turnDistance = mySpeed * 2.0f; // 적절한 선회 거리
        Vector3 scissorsPoint = myLocation + scissorsDirection * turnDistance;
        scissorsPoint.Z = myLocation.Z; // 수평 유지

        return scissorsPoint;
    }

    Vector3 Task_Scissors::CalculateScissorsEscape(CPPBlackBoard* BB)
    {
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        myForward.normalize();
        float mySpeed = BB->MySpeed_MS;
        float escapeDistance = mySpeed * 2.0f;
        Vector3 escapePoint = myLocation + myForward * escapeDistance;

        escapePoint.Z = myLocation.Z + escapeDistance * 0.1f;

        return escapePoint;
    }

    float Task_Scissors::CalculateEnergyAdvantage(CPPBlackBoard* BB)
    {
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float myAltitude = BB->MyLocation_Cartesian.Z;
        float targetAltitude = BB->TargetLocaion_Cartesian.Z;

        float myKineticEnergy = 0.5f * mySpeed * mySpeed;
        float targetKineticEnergy = 0.5f * targetSpeed * targetSpeed;
        float myPotentialEnergy = 9.81f * myAltitude;
        float targetPotentialEnergy = 9.81f * targetAltitude;

        float myTotalEnergy = myKineticEnergy + myPotentialEnergy;
        float targetTotalEnergy = targetKineticEnergy + targetPotentialEnergy;

        float energyDifference = myTotalEnergy - targetTotalEnergy;
        float averageEnergy = (myTotalEnergy + targetTotalEnergy) * 0.5f;
        
        // 상대적 에너지 우위 (비율)
        return energyDifference / averageEnergy;
    }

    float Task_Scissors::CalculateCornerSpeed(CPPBlackBoard* BB)
    {
        float baseCornerSpeed = 231.5f; // F-16 코너 속도: 약 450KCAS ≈ 231.5m/s
        
        return baseCornerSpeed;
    }

    float Task_Scissors::CalculateCornerSpeedThrottle(CPPBlackBoard* BB)
    {
        float currentThrottle = BB->Throttle;
        float currentSpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float cornerSpeed = 231.5f; // F-16 코너 속도: 약 450KCAS ≈ 231.5m/s
        
        float tempThrottle = currentThrottle;
        // 교본: "코너 속도를 유지하도록 노력한다"
        if (currentSpeed < targetSpeed - 20.0f)
        {
            tempThrottle = currentThrottle * 1.2f;
        }
        else if (currentSpeed > targetSpeed + 20.0f)
        {
            tempThrottle = currentThrottle * 0.8f;
        }
        else
        {
            return BB->Throttle;
        }
        
        return tempThrottle;
    }

    bool Task_Scissors::ShouldEscapeScissors(CPPBlackBoard* BB)
    {
        float energyAdvantage = CalculateEnergyAdvantage(BB);
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;

        bool significantEnergyAdvantage = (energyAdvantage > 0.4f);
        bool speedAdvantage = (mySpeed > targetSpeed + 50.0f);
        
        return significantEnergyAdvantage && speedAdvantage;
    }

}