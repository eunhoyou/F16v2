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
        
        BFM_Mode previousBFM = (*BB)->BFM;
   
        if (distance < 1500.0f) {
            (*BB)->BFM = NONE;  // WeaponEngagement 전용
            return NodeStatus::SUCCESS;
        }

        // 교본 기준 2nm(3704m) 이상은 턴 서클 바깥
        if (distance >= 3704.0f) {  // 턴 서클 바깥
            if (aspectAngle > 120.0f || angleOff > 135.0f) {
                (*BB)->BFM = HABFM;  // 정면 BFM
            }
            else if (aspectAngle < 30.0f) {
                (*BB)->BFM = OBFM;   // 공격 BFM (적기 후방)
            }
            else {
                (*BB)->BFM = HABFM;
            }
        }
        else {  // 턴 서클 안쪽
            if (aspectAngle > 150.0f) {
                (*BB)->BFM = DBFM;   // 방어 BFM (적기가 거의 내 후방)
            }
            else if (aspectAngle < 45.0f && angleOff < 90.0f) {
                (*BB)->BFM = OBFM;   // 공격 BFM
            }
            else if (aspectAngle > 60.0f && aspectAngle < 150.0f && 
                    angleOff > 90.0f) {
                (*BB)->BFM = SCISSORS; // 시저스 상황
            }
            else {
                (*BB)->BFM = HABFM;
            }
            
        }
        
        return NodeStatus::SUCCESS;
    }
}