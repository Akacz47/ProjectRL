import gymnasium as gym
from gymnasium import spaces
import numpy as np
import socket
from unreal_socket_client import receive_game_state, send_action

class UE5TurretEnvUDP(gym.Env):
    metadata = {"render.modes": []}

    def __init__(self):
        super().__init__()

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("127.0.0.1", 8000))

        # Obserwacja (5 wartości): 
        # [error_x, error_y, target_vel_x (v_x), target_vel_y (v_y), turret distance]
        self.observation_space = spaces.Box(
            low=np.array([-1.0, -1.0, -1.0, -1.0, 0.0], dtype=np.float32),
            high=np.array([1.0, 1.0, 1.0, 1.0, 1.0], dtype=np.float32),
            dtype=np.float32
        )

        self.action_space = spaces.Box(low=-1.0, high=1.0, shape=(2,), dtype=np.float32)

        self.bullet_distance = -1.0
        self.max_timesteps = 500
        self.reset()

    def reset(self, *, seed=None, options=None):
        super().reset(seed=seed)

        send_action(self.sock, 0, 0)

        self.hit = False
        self.step_count = 0
        return self._get_obs(), {}

    def _get_obs(self):
        obs = receive_game_state(self.sock)

        state = np.array([
            obs["error_x"],
            obs["error_y"],
            obs["v_x"],
            obs["v_y"],
            obs["target_distance"]
        ], dtype=np.float32)

        return state

    def step(self, action):
        rotateLR, rotateUD = action
        
        send_action(self.sock, rotateLR, rotateUD)

        obs = receive_game_state(self.sock)
        state = np.array([
            obs["error_x"],
            obs["error_y"],
            obs["v_x"],
            obs["v_y"],
            obs["target_distance"]
        ], dtype=np.float32)

        # Nagroda
        distance = np.linalg.norm([obs["error_x"], obs["error_y"]])
        reward = -distance * 3.0

        if obs["bullet_distance"] != self.bullet_distance and obs["bullet_distance"] > - 1.0:
            time_multiplier = max(0.2, 1.0 - (self.step_count / self.max_timesteps))

            precision_base = 1500 * np.exp(-obs["bullet_distance"] / 100.0)
            
            reward = precision_base * time_multiplier

            if int(obs["bullet_distance"]) == 0: # hit
                reward += 1500.0 * time_multiplier
                self.hit = True

            self.bullet_distance = obs["bullet_distance"]
 
        # Kara za oscylacje (nadmierne ruchy)
        reward -= 0.001 * np.linalg.norm(action)

        self.step_count += 1
        terminated = self.step_count >= self.max_timesteps or self.hit

        return state, reward, terminated, False, {}

    def render(self):
        pass

    def close(self):
        self.sock.close()
