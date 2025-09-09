#pragma once
#include "../../../Geometry/Vector3.h"
#include "../../../Geometry/EulerAngle.h"
#include <vector>

using namespace BT_Geometry;

enum BFM_Mode
{
	OBFM,
	HABFM,
	DBFM,
	DETECTING,
	SCISSORS,
	NONE
};

enum ACM_Mode
{
	EF,
	SF
};

enum TeamColor
{
	BLUE,
	RED,
	UNKNOWN
};

enum S_BFM_Mode
{
	S_OBFM,
	S_HABFM,
	S_DBFM,
	S_Others
};

enum WeaponMode
{
	Gun,
	Missile
};

/*
비행기들 객체 정보
자세, 위치, 속도, 팀, resv0(리눅스에서 ID), Resv1(비행기의 HP), Resv2(유인기/무인기)
*/
struct PlaneInfo
{
public:
	
	Vector3			Location;	//LLA Alt : Meter, 비헤비어트리로 입력할때는 LLA로 입력하지만 내부에서 사용할때는 Cartesian으로 사용
	
	EulerAngle		Rotation;	//Degree
	Vector3			AngleAcceleration;	//PQR
	
	float			Speed;		//m/s
	
	int				Team;		// 0 , 1
	float			Resv0;		//리눅스에서 ID로 쓰고있음
	float			Resv1;		//HP
	float			Resv2;		//유인기인지 무인기인지 판단용 0 : AI, 1 : Human

	PlaneInfo()
	{
		Location = Vector3(0, 0, 0);
		Rotation = EulerAngle(0, 0, 0);
		Speed = 0;
		Team = 0;
		Resv0 = 0;
		Resv1 = 0;
		Resv2 = 0;
	}
};

struct MissileTarget
{
public:
	int ListIndex;
	int DISID;
};

class CPPBlackBoard
{
public:
	CPPBlackBoard();
	~CPPBlackBoard();

public:
	double RunningTime;										//해당 시뮬레이션 실행시간
	double DeltaSecond;										//비헤비어트리 작동 틱 판단 및 시간 계산용

	std::vector<PlaneInfo> Friendly;						//아군기들 정보 Array
	std::vector<PlaneInfo> Enemy;							//적기들 정보 Array

	Vector3 MyLocation_Cartesian;							//내 위치 정보 Cartesian
	Vector3 TargetLocaion_Cartesian;						//타겟 적기 위치 정보 Cartesian
	Vector3 VP_Cartesian;									//추적점 위치 정보 Cartesian

	Vector3 MyForwardVector;								//내 전방 방향 벡터
	Vector3 MyUpVector;										//내 업 방향 벡터
	Vector3 MyRightVector;									//내 오른쪽 방향 벡터

	Vector3 TargetForwardVector;							//타겟 적기 전방 방향 벡터
	Vector3 TargetUpVector;									//타겟 적기 업 방향 벡터
	Vector3 TargetRightVector;								//타겟 적기 오른쪽 방향 벡터

	EulerAngle MyRotation_EDegree;							//내 자세, 평면 기준 자세 ,Degree
	EulerAngle TargetRotation_EDegree;						//타겟 적기 자세, 평면 기준 자세, Degree

	Vector3 MyAngleAcceleration;

	float MySpeed_MS;										//내 속도, meter/sec
	float TargetSpeed_MS;									//타겟 적기 속도. meter/sec

	float Distance;											//타겟 적기와의 거리, meter
	float Throttle;											//Throttle, 0~1


	float Los_Degree;										//타겟에 대한 LOS값
	float Los_Degree_Target;								//타겟이 나에 대한 LOS

	float MyAngleOff_Degree;								//타겟과의 기수 교차각
	float MyAspectAngle_Degree;								//타겟에 대한 AA값

	bool EnemyInSight;
	bool EnemyInSight_Target;

	BFM_Mode BFM;											//현재 BFM (OBFM, HABFM, DBFM, DETECTING, SCISSORS, NONE)
	ACM_Mode ACM;											//현재 ACM (EF, SF)


	TeamColor Team;											//팀 컬러 (BLUE, RED, UNKNOWN)


	float AltSpeed;											//고도 변화량

};