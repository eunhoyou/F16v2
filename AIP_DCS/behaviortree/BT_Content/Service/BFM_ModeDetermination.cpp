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
        
        // 이전 BFM 모드 고려 (안정성 향상)
        BFM_Mode previousBFM = (*BB)->BFM;
        
        // WEZ 체크 우선 (무기 교전)
        if (distance >= 152.4f && distance <= 914.4f && std::abs(los) <= 2.0f) {
            (*BB)->BFM = OBFM;
            return NodeStatus::SUCCESS;
        }
        
        // 단계적 BFM 모드 결정
        if (distance > 4000.0f) {
            (*BB)->BFM = HABFM;
        }
        else if (distance > 2500.0f) {
            // 중거리: AspectAngle 기반 세밀한 판단
            if (aspectAngle < 45.0f && angleOff > 120.0f) {
                (*BB)->BFM = OBFM;
            }
            else if (aspectAngle > 135.0f && angleOff < 90.0f) {
                (*BB)->BFM = DBFM;
            }
            else {
                // 이전 모드 유지 (불필요한 전환 방지)
                if (previousBFM == OBFM || previousBFM == DBFM) {
                    // 유지
                } else {
                    (*BB)->BFM = HABFM;
                }
            }
        }
        else if (distance > 800.0f) {
            // 근거리: 더 엄격한 조건
            if (aspectAngle < 30.0f && angleOff > 150.0f) {
                (*BB)->BFM = OBFM;
            }
            else if (aspectAngle > 150.0f && angleOff < 60.0f) {
                (*BB)->BFM = DBFM;
            }
            else {
                (*BB)->BFM = SCISSORS;
            }
        }
        else {
            // 초근거리: 무조건 SCISSORS
            (*BB)->BFM = SCISSORS;
        }
        
        return NodeStatus::SUCCESS;
    }
}