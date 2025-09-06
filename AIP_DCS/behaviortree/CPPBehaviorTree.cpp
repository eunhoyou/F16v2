// Fill out your copyright notice in the Description page of Project Settings.


#include "CPPBehaviorTree.h"


Vector3 UCPPBehaviorTree::LLAtoCartesian(Vector3 LLA, Vector3 BaseLLA)
{
	double eccentricitysquare, N, M;
	eccentricitysquare = 1.0 - pow(6356752.3142, 2) / pow(6378137.0, 2);
	N = 6378137.0 / sqrt(1.0 - eccentricitysquare * pow(sin(BaseLLA.X * PI / 180.0), 2)); // prime vertical radius of curvature
	M = 6378137.0 * (1.0 - eccentricitysquare) / pow(1 - eccentricitysquare * pow(sin(BaseLLA.X * PI / 180.0), 2), 3 / 2);

	double dlat, dlon;
	dlat = LLA.X - BaseLLA.X;
	dlon = LLA.Y - BaseLLA.Y;

	double dN, dE, dD;
	dN = (M + BaseLLA.Z) * dlat * PI / 180.0;
	dE = (N + BaseLLA.Z) * cos(BaseLLA.X * PI / 180.0) * dlon * PI / 180.0;
	dD = (LLA.Z - BaseLLA.Z);
	Vector3 res(dN, dE, dD);
	return res;
}

// Sets default values for this component's properties
UCPPBehaviorTree::UCPPBehaviorTree()
{

	f2m = 3.28084;
	EQ_R = 6.378137E+6;
	P_R = 6.3567523142E+6;
	fr = 298.257223563;
	Req = 6.378137E+6;
	d2r = 3.1415926535897931 / 180.0;
	m2f = 3.28084;


	elev0 = 0.2;
	aile0 = 0.0;
	eccen = 1.0 - P_R * P_R / (EQ_R * EQ_R);

	BB = new CPPBlackBoard();

	//std::cout << "Behavior Tree Version : 2022.07.11" << std::endl;
}


UCPPBehaviorTree::~UCPPBehaviorTree()
{
	delete BB;
}


void UCPPBehaviorTree::init()
{
	last_log_time = 0.0;
    log_interval = 1;
	/*
	노드 입력 : 구현해둔 노드들을 Factory 객체에 입력해주는 과정
	*/

	Factory.registerNodeType<Action::SelectTarget>("SelectTarget");
	Factory.registerNodeType<Action::DistanceUpdate>("DistanceUpdate");
	Factory.registerNodeType<Action::CheckSight>("CheckSight");
	Factory.registerNodeType<Action::AngleOffUpdate>("AngleOffUpdate");
	Factory.registerNodeType<Action::DirectionVectorUpdate>("DirectionVectorUpdate");
	Factory.registerNodeType<Action::AspectAngleUpdate>("AspectAngleUpdate");
	Factory.registerNodeType<Action::BFM_ModeDetermination>("BFM_ModeDetermination");
	Factory.registerNodeType<Action::AltitudeSafetyCheck>("AltitudeSafetyCheck");

	Factory.registerNodeType<Action::DECO_BFMCheck>("DECO_BFMCheck");
	Factory.registerNodeType<Action::DECO_DistanceCheck>("DECO_DistanceCheck");
	Factory.registerNodeType<Action::DECO_AngleOffCheck>("DECO_AngleOffCheck");
	Factory.registerNodeType<Action::DECO_LOSCheck>("DECO_LOSCheck");

	Factory.registerNodeType<Action::Task_HeadOnBFM>("Task_HeadOnBFM");
	Factory.registerNodeType<Action::Task_OffensiveBFM>("Task_OffensiveBFM");
	Factory.registerNodeType<Action::Task_DefensiveBFM>("Task_DefensiveBFM");
	Factory.registerNodeType<Action::Task_WeaponEngagement>("Task_WeaponEngagement");
	Factory.registerNodeType<Action::Task_Scissors>("Task_Scissors");

	// //파일로 트리 구조 정의
	tree = Factory.createTreeFromFile("./KAURML.xml");
	tree.rootBlackboard()->set<CPPBlackBoard*>("BB", BB);
}

StickValue UCPPBehaviorTree::Step(PlaneInfo MyInfo, int NumofOtherPlane, PlaneInfo* OthersInfo, Vector3 & VP, float & Throttle)
{
	/*LLA 좌표를 Cartesian 좌표로 변경
			
	굳이 리눅스의 기준 좌표 37, 127로 맞출 필요 없음
	*/
	Vector3 Mylocation_Cartesian = LLAtoCartesian(MyInfo.Location, Vector3(OriLAT, OriLOn, 0));

	//Cartesiam 좌표계로 위치 정보를 바꾼 내 비행기 정보
	PlaneInfo Myinfo;
	Myinfo.Location = Mylocation_Cartesian;
	Myinfo.Rotation = EulerAngle(MyInfo.Rotation.Yaw, MyInfo.Rotation.Pitch, MyInfo.Rotation.Roll);
	Myinfo.AngleAcceleration = MyInfo.AngleAcceleration;
	Myinfo.Speed = MyInfo.Speed;
	Myinfo.Team = MyInfo.Team;
	Myinfo.Resv0 = MyInfo.Resv0;		//ID
	Myinfo.Resv1 = MyInfo.Resv1;		//HP
	Myinfo.Resv2 = MyInfo.Resv2;		//OperationMode

	//HP가 0이하가 되면 자유 낙하하도록 설정
	if (Myinfo.Resv1 <= 0)
	{
		StickValue R;

		BB->VP_Cartesian = Vector3(BB->MyLocation_Cartesian.X, BB->MyLocation_Cartesian.Y, 0);
		R = Controller.GetStick(
			BB->MyLocation_Cartesian,
			Vector3(BB->MyRotation_EDegree.Roll*DEG2RAD,
				BB->MyRotation_EDegree.Pitch*DEG2RAD,
				BB->MyRotation_EDegree.Yaw*DEG2RAD),
			BB->VP_Cartesian);
		BB->Throttle = 0;
		R.RudderCMD = 100;

		std::cout << " HP : 0 !!!!!!!!!" << std::endl;
		return R;
	}

	//HP가 0이상일때
	else
	{
		//다른 비행기들 위치 좌표계 변환
		PlaneInfo others[4];
		for (int i = 0; i < NumofOtherPlane; i++)
		{
			Vector3 Enemylocation_Cartesian = LLAtoCartesian(OthersInfo[i].Location, Vector3(OriLAT, OriLOn, 0));
			others[i].Location = Enemylocation_Cartesian;
			others[i].Rotation = EulerAngle(OthersInfo[i].Rotation.Yaw, OthersInfo[i].Rotation.Pitch, OthersInfo[i].Rotation.Roll);
			others[i].Speed = OthersInfo[i].Speed;
			others[i].Team = OthersInfo[i].Team;
			others[i].Resv0 = OthersInfo[i].Resv0;
			others[i].Resv1 = OthersInfo[i].Resv1;
			others[i].Resv2 = OthersInfo[i].Resv2;
			
			
		}

		//블랙보드의 아군기, 적군기 List 초기화
		BB->Friendly.clear();
		BB->Enemy.clear();

		//블랙보드에 내 정보(위치, 자세, 속력, 팀) 업데이트
		BB->MyLocation_Cartesian = Mylocation_Cartesian;
		BB->MyRotation_EDegree = EulerAngle(Myinfo.Rotation.Yaw, Myinfo.Rotation.Pitch, Myinfo.Rotation.Roll);
		BB->MyAngleAcceleration = Myinfo.AngleAcceleration;
		BB->MySpeed_MS = Myinfo.Speed;
		BB->Team = (TeamColor)Myinfo.Team;

		//아군기 리스트에 내 정보 추가. Friendly의 index 0번은 무조건 나 자신
		BB->Friendly.push_back(Myinfo);

		//생존중인 비행기들의 적아 구분
		for (int i = 0; i < NumofOtherPlane; i++)
		{
			if (others[i].Resv1 > 0)
			{
				if (others[i].Team == Myinfo.Team)
				{
					BB->Friendly.push_back(others[i]);
				}
				else
				{
					BB->Enemy.push_back(others[i]);
				}
			}
			else
			{

			}
		}


		bool AimmingMode;

		StickValue R;

		//블랙보드에 입력된 정보를 바탕으로 비헤비어트리 Run
		RunCPPBT(VP, Throttle, AimmingMode);

		// //임시 코드
		// Throttle = 1.0;

		R = Controller.GetStick(
			BB->MyLocation_Cartesian,
			Vector3(BB->MyRotation_EDegree.Roll*DEG2RAD,
					BB->MyRotation_EDegree.Pitch*DEG2RAD,
					BB->MyRotation_EDegree.Yaw*DEG2RAD),
				VP);

 
		
		return R;
	}
}

Vector3 UCPPBehaviorTree::GetVP()
{
	Vector3 Vp = (*BB).VP_Cartesian;
	return Vp;
}


void UCPPBehaviorTree::RunCPPBT(Vector3& VP, float& Throttle, bool& AimmingMode)
{
    BB->RunningTime += BB->DeltaSecond;
    
    // 0.5초마다 로그 기록
    bool should_log = (BB->RunningTime - last_log_time) >= log_interval;
    
    // BT 실행은 매번
    NodeStatus result = tree.tickRoot();
    
    VP = Vector3(BB->VP_Cartesian.X, BB->VP_Cartesian.Y, BB->VP_Cartesian.Z);
    Throttle = BB->Throttle;
    
    // 로그는 0.5초 주기로만 기록
    if (should_log) {
        last_log_time = BB->RunningTime;
        
        // 항공기별 로그 파일명 생성
        std::string log_filename = "aircraft_" + std::to_string(ID) + "_debug.log";
        
        std::ofstream log_file(log_filename, std::ios::app);
        if (log_file.is_open()) {
            log_file << "=====================================\n";
            log_file << "[Aircraft ID:" << ID << "] [Time:" << BB->RunningTime << "s]\n";
            
            // 내 위치 정보
            log_file << "[MyPos: (" << BB->MyLocation_Cartesian.X << ", " 
                     << BB->MyLocation_Cartesian.Y << ", " 
                     << BB->MyLocation_Cartesian.Z << ")]\n";
            
            // 적기 정보
            if(BB->Enemy.size() > 0) {
                log_file << "[EnemyPos: (" << BB->Enemy[0].Location.X << ", " 
                         << BB->Enemy[0].Location.Y << ", " 
                         << BB->Enemy[0].Location.Z << ")]\n";
                log_file << "[Distance: " << BB->Distance << "m]\n";
                log_file << "[AspectAngle: " << BB->MyAspectAngle_Degree << "]\n";
                log_file << "[AngleOff: " << BB->MyAngleOff_Degree << "]\n";
                log_file << "[LOS: " << BB->Los_Degree << "]\n";
            }
            
            // BFM 모드 정보
            std::string bfm_mode;
            switch(BB->BFM) {
                case OBFM: bfm_mode = "OBFM"; break;
                case HABFM: bfm_mode = "HABFM"; break;
                case DBFM: bfm_mode = "DBFM"; break;
                case SCISSORS: bfm_mode = "SCISSORS"; break;
                case DETECTING: bfm_mode = "DETECTING"; break;
                case NONE: bfm_mode = "NONE"; break;
                default: bfm_mode = "UNKNOWN"; break;
            }
            log_file << "[BFM Mode: " << bfm_mode << "]\n";
            
            // 속도 정보
            log_file << "[MySpeed: " << BB->MySpeed_MS << "m/s]\n";
            if(BB->Enemy.size() > 0) {
                log_file << "[TargetSpeed: " << BB->TargetSpeed_MS << "m/s]\n";
            }
            
            // VP와 Throttle 정보도 같은 주기로 기록
            log_file << "[VP: (" << VP.X << ", " << VP.Y << ", " << VP.Z << ")]\n";
            log_file << "[Throttle: " << Throttle << "]\n";
            log_file << "=====================================\n\n";
            
            log_file.close();
        }
        
        // 콘솔 출력도 0.5초 주기로
        std::cout << "[ID:" << ID << "] Running time: " << BB->RunningTime << std::endl;
    }
}

 void UCPPBehaviorTree::SetDeltaTime(double DT)
 {
	 BB->DeltaSecond = DT;
 }