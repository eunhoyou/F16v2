#include "BFM_ModeDetermination.h"

namespace Action
{
    PortsList BFM_ModeDetermination::providedPorts()
    {
        return {
            InputPort<CPPBlackBoard*>("BB")
        };
    }

    NodeStatus BFM_ModeDetermination::tick()
    {
        Optional<CPPBlackBoard*> BB = getInput<CPPBlackBoard*>("BB");

        float distance = (*BB)->Distance;
        float aspectAngle = (*BB)->MyAspectAngle_Degree;
        float angleOff = (*BB)->MyAngleOff_Degree;
        float los = (*BB)->Los_Degree;
        
        // 이전 BFM 모드 (안정성)
        BFM_Mode previousBFM = (*BB)->BFM;
        
        std::cout << "[BFM_ModeDetermination] Distance: " << distance 
                  << ", AspectAngle: " << aspectAngle 
                  << ", AngleOff: " << angleOff 
                  << ", LOS: " << los << std::endl;
        
        // 1순위: WEZ 내 무기 교전 (최우선)
        if (IsInWEZ(distance, los)) {
            std::cout << "[BFM_ModeDetermination] IN WEZ - Weapon Engagement" << std::endl;
            (*BB)->BFM = OBFM; // WEZ에서는 공격 BFM으로 분류
            return NodeStatus::SUCCESS;
        }
        
        // 2순위: 교본의 턴 서클 개념 적용
        TurnCircleRelation circleRelation = DetermineTurnCircleRelation(BB.value());
        
        switch (circleRelation)
        {
            case INSIDE_TARGET_CIRCLE:
                // 교본: "적기의 턴 서클 안에 있으면 공격 가능"
                if (aspectAngle < 45.0f) { // 적기 후방
                    (*BB)->BFM = OBFM;
                    std::cout << "[BFM_ModeDetermination] Inside target circle - OFFENSIVE" << std::endl;
                } else if (aspectAngle > 135.0f) { // 적기 전방
                    (*BB)->BFM = DBFM;
                    std::cout << "[BFM_ModeDetermination] Inside target circle, high aspect - DEFENSIVE" << std::endl;
                } else {
                    (*BB)->BFM = SCISSORS; // 중립 위치
                    std::cout << "[BFM_ModeDetermination] Inside target circle, neutral - SCISSORS" << std::endl;
                }
                break;
                
            case OUTSIDE_TARGET_CIRCLE:
                // 교본: "적기의 턴 서클 밖에 있으면 정면 BFM"
                if (IsHeadOnSituation(aspectAngle, angleOff)) {
                    (*BB)->BFM = HABFM;
                    std::cout << "[BFM_ModeDetermination] Outside circle, head-on - HEAD-ON BFM" << std::endl;
                } else if (aspectAngle > 150.0f && angleOff < 60.0f) {
                    (*BB)->BFM = DBFM; // 적기가 나를 추적 중
                    std::cout << "[BFM_ModeDetermination] Outside circle, being tracked - DEFENSIVE" << std::endl;
                } else {
                    // 모호한 상황 - 이전 모드 유지 또는 기본값
                    if (previousBFM != NONE) {
                        // 유지
                        std::cout << "[BFM_ModeDetermination] Ambiguous situation, maintaining previous mode" << std::endl;
                    } else {
                        (*BB)->BFM = HABFM; // 기본값
                        std::cout << "[BFM_ModeDetermination] Ambiguous situation, default HEAD-ON" << std::endl;
                    }
                }
                break;
                
            case ON_CIRCLE_EDGE:
                // 턴 서클 경계 - 거리와 각도로 세밀 판단
                if (distance < 1000.0f) {
                    (*BB)->BFM = SCISSORS;
                    std::cout << "[BFM_ModeDetermination] Circle edge, close - SCISSORS" << std::endl;
                } else {
                    (*BB)->BFM = HABFM;
                    std::cout << "[BFM_ModeDetermination] Circle edge, distant - HEAD-ON" << std::endl;
                }
                break;
        }
        
        return NodeStatus::SUCCESS;
    }

    BFM_ModeDetermination::TurnCircleRelation BFM_ModeDetermination::DetermineTurnCircleRelation(CPPBlackBoard* BB)
    {
        float distance = BB->Distance;
        float targetSpeed = BB->TargetSpeed_MS;
        float aspectAngle = BB->MyAspectAngle_Degree;
        
        // 교본: 적기의 예상 턴 서클 계산
        float estimatedTargetTurnRadius = EstimateTargetTurnRadius(BB);
        
        // 적기가 최대 G로 선회할 때의 선회원 고려
        // aspectAngle이 낮으면 적기 후방에서 추적 중
        if (aspectAngle < 30.0f && distance < estimatedTargetTurnRadius * 1.5f) {
            return INSIDE_TARGET_CIRCLE;
        }
        // 매우 높은 aspect는 정면 또는 적기가 나를 추적 중
        else if (aspectAngle > 150.0f) {
            if (distance < estimatedTargetTurnRadius * 2.0f) {
                return INSIDE_TARGET_CIRCLE; // 방어 상황
            } else {
                return OUTSIDE_TARGET_CIRCLE; // 정면 상황
            }
        }
        // 중간 aspect - 거리로 판단
        else if (distance > estimatedTargetTurnRadius * 2.5f) {
            return OUTSIDE_TARGET_CIRCLE;
        }
        else if (distance < estimatedTargetTurnRadius * 0.8f) {
            return INSIDE_TARGET_CIRCLE;
        }
        else {
            return ON_CIRCLE_EDGE;
        }
    }

    float BFM_ModeDetermination::EstimateTargetTurnRadius(CPPBlackBoard* BB)
    {
        float targetSpeed = BB->TargetSpeed_MS;
        
        // 교본 공식: TR = V²/(g*G)
        // 적기가 7G로 선회한다고 가정 (일반적인 BFM)
        float assumedGLoad = 7.0f;
        float turnRadius = (targetSpeed * targetSpeed) / (9.81f * assumedGLoad);
        
        // 최소/최대 제한 (비현실적인 값 방지)
        turnRadius = std::max(200.0f, std::min(turnRadius, 2000.0f));
        
        std::cout << "[EstimateTargetTurnRadius] Speed: " << targetSpeed 
                  << "m/s, Estimated radius: " << turnRadius << "m" << std::endl;
        
        return turnRadius;
    }

    bool BFM_ModeDetermination::IsHeadOnSituation(float aspectAngle, float angleOff)
    {
        // 교본: 정면 상황의 조건
        // 1. 높은 aspect (서로 마주보고 있음)
        // 2. 높은 angle off (서로 다른 방향을 향함)
        bool highAspect = (aspectAngle > 120.0f && aspectAngle < 240.0f);
        bool convergingCourse = (angleOff > 90.0f);
        
        return highAspect && convergingCourse;
    }

    bool BFM_ModeDetermination::IsInWEZ(float distance, float los)
    {
        // 시뮬레이터 WEZ 조건
        bool rangeValid = (distance >= WEZ_MIN_RANGE && distance <= WEZ_MAX_RANGE);
        bool angleValid = (std::abs(los) <= WEZ_MAX_ANGLE);
        
        return rangeValid && angleValid;
    }
}