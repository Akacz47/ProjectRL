import gymnasium as gym
from gymnasium import spaces
import numpy as np
import socket
import json
from unreal_socket_client import receive_game_state, send_action, receive_game_state2, send_action2

# BulletDistance = 0 to hit,
# To idzie w pakiecie z ue5
# self.target_vel_x = 0.0
# self.target_vel_y = 0.0

# pakiet zawiera: [error_x, error_y, target_vel_x (v_x), target_vel_y (v_y)] + bulletDistance
class UE5TurretEnv(gym.Env):
    metadata = {"render.modes": []}

    def __init__(self):
        super().__init__()

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("127.0.0.1", 8000))

        # OBSERWACJA (4 wartości): 
        # [error_x, error_y, target_vel_x (v_x), target_vel_y (v_y)] # ew. turret distance od 0 do 1
        self.observation_space = spaces.Box(
            low=np.array([-1.0, -1.0, -1.0, -1.0, 0.0, 0.0], dtype=np.float32),
            high=np.array([1.0, 1.0, 1.0, 1.0, 1.0, 1.0], dtype=np.float32),
            dtype=np.float32
        )

        self.action_space = spaces.Box(low=-1.0, high=1.0, shape=(2,), dtype=np.float32)

        self.bullet_distance = -1.0
        self.reset()

    def reset(self, *, seed=None, options=None):
        super().reset(seed=seed)

        # send_action(self.sock, {
        #     "rotateLeftRight": 0,
        #     "rotateUpDown": 0,
        #     "reset": True
        # })
        send_action2(self.sock, 0, 0)

        self.hit = False
        self.step_count = 0
        return self._get_obs(), {}

    def _get_obs(self):
        #print('Czekam na odp')
        obs = receive_game_state2(self.sock)

        #print(f'Reward: {obs["reward"]} BulletDistance: {obs["bullet_distance"]}')
        #print(f'Reward: {obs["reward"]}')

        state = np.array([
            obs["error_x"],
            obs["error_y"],
            obs["v_x"],
            obs["v_y"],
            1.0 - np.exp(-obs["target_distance"] / 4500.0),
            obs["shoot_cooldown"]
        ], dtype=np.float32)

        return state

    def step(self, action):
        rotateLR, rotateUD = action
        
        # send_action(self.sock, {
        #     "rotateLeftRight": float(rotateLR),
        #     "rotateUpDown": float(rotateUD)
        # })
        send_action2(self.sock, rotateLR, rotateUD)

        obs = receive_game_state2(self.sock)
        state = np.array([
            obs["error_x"],
            obs["error_y"],
            obs["v_x"],
            obs["v_y"],
            1.0 - np.exp(-obs["target_distance"] / 4500.0),
            obs["shoot_cooldown"]
        ], dtype=np.float32)
        #print(obs["shoot_cooldown"])

        # --- 3. NAGRODA ---
        distance = np.linalg.norm([obs["error_x"], obs["error_y"]])

        reward = -15.0 * (distance ** 2)
        
        # 2. BONUS ZA LEAD (Wyprzedzanie):
        if obs["bullet_distance"] != self.bullet_distance and obs["bullet_distance"] > - 0.5:
            time_multiplier = max(0.2, 1.0 - (self.step_count / 500.0))

            precision_base = 1500 * np.exp(-obs["bullet_distance"] / 100.0)
            
            # Dodajemy bonus czasowy do trafienia i precyzji
            reward += precision_base * time_multiplier

            if int(obs["bullet_distance"]) == 0: # hit
                reward += 2000.0 * time_multiplier
                self.hit = True

            self.bullet_distance = obs["bullet_distance"]
 
        # Kara za oscylacje (nadmierne ruchy)
        reward -= 0.5 * np.linalg.norm(action) # - 0.05
        
        # Kara za nie kończenie epizodu
        reward -= 2.5 # -1.0

        self.step_count += 1
        terminated = self.step_count >= 500 or self.hit # 500

        return state, reward, terminated, False, {}

    def render(self):
        pass

    def close(self):
        self.sock.close()


# Test środowiska
# if __name__ == "__main__":
#     env = UE5TurretEnvOffline()
#     obs, _ = env.reset()
#     print("Reset:", obs)
#     for _ in range(10):
#         action = env.action_space.sample()
#         obs, reward, done, truncated, info = env.step(action)
#         print("Step:", obs, reward, done)


# Test środowiska V2
# from stable_baselines3 import SAC
# env = UE5TurretEnvOffline()
# model = SAC.load("sac_turret_model_offline", env=env)
# print(f'{model._total_timesteps=}')
# obs, _ = env.reset()
# print("Reset:", obs)

# for _ in range(30):
#     action, _ = model.predict(obs, deterministic=True)
#     obs, reward, done, truncated, info = env.step(action)
#     print("Step:", obs, "Reward:", reward, "done:", done, "action:", action)

#     if done or truncated:
#         obs, _ = env.reset()
#         print("Reset:", obs)

# env.close()
