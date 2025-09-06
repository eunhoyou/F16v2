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
        Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");
        
        float distance = (*BB)->Distance;
        float mySpeed = (*BB)->MySpeed_MS;
        float targetSpeed = (*BB)->TargetSpeed_MS;
        float myAltitude = static_cast<float>(std::abs((*BB)->MyLocation_Cartesian.Z));
        float los = (*BB)->Los_Degree;

        std::cout << "[Task_HeadOnBFM] Distance: " << distance 
                  << "m, MySpeed: " << mySpeed 
                  << "m/s, TargetSpeed: " << targetSpeed 
                  << "m/s, LOS: " << los << "°" << std::endl;

        // 교본: 에스케이프 윈도우 확인 (거리와 속도 우위 기반)
        if (ShouldDisengage(BB.value()))
        {
            std::cout << "[Task_HeadOnBFM] Escape window open - disengaging" << std::endl;
            (*BB)->VP_Cartesian = CalculateDisengagement(BB.value());
            (*BB)->Throttle = 1.0f;
            return NodeStatus::SUCCESS;
        }

        // 교본: 리드 턴 시점 판단 (거리와 각속도 기반)
        if (ShouldInitiateLeadTurn(BB.value()))
        {
            std::cout << "[Task_HeadOnBFM] Initiating lead turn" << std::endl;
            (*BB)->VP_Cartesian = CalculateLeadTurn(BB.value());
            (*BB)->Throttle = 0.9f;
        }
        else
        {
            // 교본: 교차 시 기동 선택 (속도 차이와 상황에 따라)
            float speedDifference = mySpeed - targetSpeed;
            
            if (speedDifference > 30.0f)
            {
                // 속도 우위: 슬라이스 턴 (기수 아래로 하강 선회)
                std::cout << "[Task_HeadOnBFM] Speed advantage - executing slice turn" << std::endl;
                (*BB)->VP_Cartesian = CalculateSliceTurn(BB.value());
                (*BB)->Throttle = 0.8f;
            }
            else if (speedDifference < -30.0f && myAltitude > 2000.0f)
            {
                // 속도 열세 + 충분한 고도: 수직 기동
                std::cout << "[Task_HeadOnBFM] Speed disadvantage - executing vertical maneuver" << std::endl;
                (*BB)->VP_Cartesian = CalculateVerticalManeuver(BB.value());
                (*BB)->Throttle = 1.0f;
            }
            else
            {
                // 비슷한 속도 또는 제한된 고도: 수평 선회
                std::cout << "[Task_HeadOnBFM] Similar speeds - executing level turn" << std::endl;
                (*BB)->VP_Cartesian = CalculateLevelTurn(BB.value());
                (*BB)->Throttle = 0.9f;
            }
        }

        return NodeStatus::SUCCESS;
    }

    Vector3 Task_HeadOnBFM::CalculateSliceTurn(CPPBlackBoard* BB)
    {
        // 교본: 기수를 아래로 하고 높은 G로 리드 턴
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        Vector3 myRight = BB->MyRightVector;
        float mySpeed = BB->MySpeed_MS;
        float distance = BB->Distance;

        // 적기 방향으로 선회 방향 결정
        Vector3 toTarget = (targetLocation - myLocation);
        toTarget = toTarget / distance; // 정규화
        float dotRight = toTarget.dot(myRight);
        Vector3 sliceDirection = (dotRight > 0) ? myRight : myRight * -1.0f;

        // 코너 속도 기반 선회 거리 계산
        float turnRadius = (mySpeed * mySpeed) / (9.81f * 8.0f); // 8G 선회
        float sliceDistance = turnRadius * 1.5f;

        Vector3 slicePoint = myLocation + sliceDirection * sliceDistance + myForward * (mySpeed * 3.0f);
        
        // 교본: 기수를 10도 정도 아래로 (중력 이용)
        float descentRate = 150.0f; // 150m 하강
        slicePoint.Z = myLocation.Z + descentRate; // NED 좌표계

        std::cout << "[CalculateSliceTurn] Turn radius: " << turnRadius 
                  << "m, Slice distance: " << sliceDistance << "m" << std::endl;

        return slicePoint;
    }

    Vector3 Task_HeadOnBFM::CalculateLevelTurn(CPPBlackBoard* BB)
    {
        // 교본: 수평 선회 (적기를 시야에 유지 가능)
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 myRight = BB->MyRightVector;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;
        float distance = BB->Distance;

        Vector3 toTarget = (targetLocation - myLocation);
        toTarget = toTarget / distance;
        float dotRight = toTarget.dot(myRight);
        Vector3 turnDirection = (dotRight > 0) ? myRight : myRight * -1.0f;

        // 6-7G 수평 선회
        float turnRadius = (mySpeed * mySpeed) / (9.81f * 6.5f);
        float turnDistance = turnRadius * 1.2f;

        Vector3 levelTurnPoint = myLocation + turnDirection * turnDistance + myForward * (mySpeed * 2.5f);
        levelTurnPoint.Z = myLocation.Z; // 수평 유지

        std::cout << "[CalculateLevelTurn] Turn distance: " << turnDistance << "m" << std::endl;

        return levelTurnPoint;
    }

    Vector3 Task_HeadOnBFM::CalculateVerticalManeuver(CPPBlackBoard* BB)
    {
        // 교본: 뱅크를 수평으로 풀고 수직으로 기수를 당김
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        Vector3 myUp = BB->MyUpVector;
        float mySpeed = BB->MySpeed_MS;
        float currentAltitude = static_cast<float>(std::abs(myLocation.Z));

        // 교본: 550노트 기준으로 수직 기동 가능성 판단
        float minVerticalSpeed = 160.0f; // m/s (약 311노트)
        
        if (mySpeed >= minVerticalSpeed)
        {
            // 수직 상승 거리 계산 (에너지 보존 법칙)
            float climbHeight = (mySpeed * mySpeed) / (2.0f * 9.81f) * 0.4f; // 40% 에너지 사용
            climbHeight = std::min(climbHeight, 800.0f); // 최대 800m 상승

            Vector3 verticalPoint = myLocation + myUp * (-climbHeight) + myForward * (mySpeed * 2.0f);
            
            std::cout << "[CalculateVerticalManeuver] Climb height: " << climbHeight << "m" << std::endl;
            return verticalPoint;
        }
        else
        {
            // 속도 부족시 수평 선회로 대체
            std::cout << "[CalculateVerticalManeuver] Insufficient speed, falling back to level turn" << std::endl;
            return CalculateLevelTurn(BB);
        }
    }

    Vector3 Task_HeadOnBFM::CalculateLeadTurn(CPPBlackBoard* BB)
    {
        // 교본: 적기의 각속도가 빨라질 때 리드 턴 시작
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 targetLocation = BB->TargetLocaion_Cartesian;
        Vector3 targetForward = BB->TargetForwardVector;
        Vector3 myRight = BB->MyRightVector;
        float targetSpeed = BB->TargetSpeed_MS;
        float distance = BB->Distance;

        // 적기의 예측 위치 (2-3초 후)
        float leadTime = 2.5f;
        Vector3 predictedTargetPos = targetLocation + targetForward * targetSpeed * leadTime;

        // 리드 턴 방향 결정
        Vector3 toPredict = (predictedTargetPos - myLocation);
        toPredict = toPredict / distance;
        float dotRight = toPredict.dot(myRight);
        Vector3 leadDirection = (dotRight > 0) ? myRight : myRight * -1.0f;

        // 최대 G로 리드 턴
        float leadDistance = distance * 0.4f; // 거리의 40%
        Vector3 leadTurnPoint = myLocation + leadDirection * leadDistance;

        std::cout << "[CalculateLeadTurn] Lead time: " << leadTime 
                  << "s, Lead distance: " << leadDistance << "m" << std::endl;

        return leadTurnPoint;
    }

    Vector3 Task_HeadOnBFM::CalculateDisengagement(CPPBlackBoard* BB)
    {
        // 교본: 에스케이프 윈도우가 열려있을 때 이탈
        Vector3 myLocation = BB->MyLocation_Cartesian;
        Vector3 myForward = BB->MyForwardVector;
        float mySpeed = BB->MySpeed_MS;

        // 현재 방향으로 고속 이탈
        float escapeDistance = mySpeed * 8.0f; // 8초간 이동 거리
        Vector3 escapePoint = myLocation + myForward * escapeDistance;

        std::cout << "[CalculateDisengagement] Escape distance: " << escapeDistance << "m" << std::endl;

        return escapePoint;
    }

    bool Task_HeadOnBFM::ShouldInitiateLeadTurn(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float los = BB->Los_Degree;

        // 교본: 적기가 캐노피 옆으로 움직이기 시작할 때 (각속도 증가)
        // 거리와 LOS 기반으로 판단
        bool distanceCheck = (distance < 3000.0f && distance > 1000.0f);
        bool angleCheck = (std::abs(los) < 30.0f); // 30도 이내에서 리드 턴

        std::cout << "[ShouldInitiateLeadTurn] Distance: " << distanceCheck 
                  << ", Angle: " << angleCheck << std::endl;

        return distanceCheck && angleCheck;
    }

    bool Task_HeadOnBFM::ShouldDisengage(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float mySpeed = BB->MySpeed_MS;
        float targetSpeed = BB->TargetSpeed_MS;
        float angleOff = BB->MyAngleOff_Degree;

        // 교본: 에스케이프 윈도우 조건
        // 1. 충분한 거리 (2km 이상)
        // 2. 속도 우위 (15m/s 이상)
        // 3. 높은 앵글 오프 (90도 이상)
        bool distanceOk = (distance > 2000.0f);
        bool speedAdvantage = (mySpeed - targetSpeed > 15.0f);
        bool angleOk = (angleOff > 90.0f);

        std::cout << "[ShouldDisengage] Distance: " << distanceOk 
                  << ", Speed: " << speedAdvantage 
                  << ", Angle: " << angleOk << std::endl;

        return distanceOk && speedAdvantage && angleOk;
    }
}