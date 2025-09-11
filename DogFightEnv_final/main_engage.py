import numpy as np
import os
import sys
from DogFightEnvWrapper import DogFightWrapper
import CppBT

if __name__ == "__main__":
    env_config = {
        'max_engage_time': 300,
        'min_altitude': 300,
        # [N, E, D, Roll, Pitch, Initial Heading(deg), Speed(m/s)]
        'ownship': [1000.0, 0.0, -7000.0, 0.0, 0.0, 180.0, 300.0],
        'target': [6000.0, 0.0, -7000.0, 0.0, 0.0, 0.0, 300.0],
    }

    AIP_ownship = CppBT.AIPilot("KAU_RML.dll")
    AIP_target = CppBT.AIPilot("KAU_RML.dll")
    engageEnv = DogFightWrapper(env_config, AIP_ownship, AIP_target)
        
    engageEnv.reset()
    obs = None
    done = False
    reward = 0.0
    step_count = 0
    fuel_log = []
    while not done:
        obs, reward, done, info = engageEnv.step()
        
        # 매 10스텝마다 연료 정보 출력
        if step_count % 10 == 0:
            ownship_fuel = engageEnv.get_ownship_state()[23]
            target_fuel = engageEnv.get_target_state()[23]
            sim_time = engageEnv.get_ownship_state()[41]
            
            print(f"Step: {step_count}, Time: {sim_time:.2f}s, "
                  f"Ownship Fuel: {ownship_fuel:.2f} lbs, "
                  f"Target Fuel: {target_fuel:.2f} lbs")
            
            # 로그에 저장
            fuel_log.append([step_count, sim_time, ownship_fuel, target_fuel])
        
        step_count += 1
        
    if done:
        engageEnv.make_tacviewLog()