import gymnasium as gym
from gymnasium import spaces
import numpy as np

class UE5TurretEnvSimple(gym.Env):
    metadata = {"render.modes": []}

    def __init__(self):
        super().__init__()

        # OBSERWACJA (4 wartości): 
        # [error_x, error_y, target_vel_x (v_x), target_vel_y (v_y)]
        self.observation_space = spaces.Box(
            low=np.array([-1.0, -1.0, -1.0, -1.0, 0.0], dtype=np.float32),
            high=np.array([1.0, 1.0, 1.0, 1.0, 1.0], dtype=np.float32),
            dtype=np.float32
        )

        self.action_space = spaces.Box(low=-1.0, high=1.0, shape=(2,), dtype=np.float32)

        # Parametry ruchu
        self.correction_strength = 1.0 # 0.05
        self.hitting_counter = 0
        #self.max_ue_distance = 6000.0
        self.max_steps = 500
        self.reset()

    def reset(self, *, seed=None, options=None):
        super().reset(seed=seed)

        # 1. Pozycja lufy wieżyczki
        self.turret_x = 0.0
        self.turret_y = 0.0

        # 2. Losowa pozycja startowa celu
        r = np.random.uniform(0.5, 1.0)
        angle = np.random.uniform(0, 2 * np.pi)
        self.target_x = float(r * np.cos(angle))
        self.target_y = float(r * np.sin(angle))

        self.target_vel_x = 0.0
        self.target_vel_y = 0.0

        # 3. Losowy styl ruchu celu dla uniwersalności
        self.movement_type = np.random.choice(["sinusoid", "linear"])
        self.target_speed = np.random.uniform(0.0, 0.075) # 0.03, 0.055
        self.move_angle = np.random.uniform(0, 2 * np.pi)
        self.phase = 0.0

        self.turret_distance = 1.0

        self.step_count = 0
        self.hitting_counter = 0
        return self._get_obs(), {}

    def _get_obs(self):
        # W Pythonie symulujemy to, co w C++ robią funkcje GetAngleDelta
        # Zamiast surowej różnicy, używamy sinusa dla X i normalizacji dla Y
        error_x = np.sin(self.target_x - self.turret_x) 
        error_y = (self.target_y - self.turret_y) / (np.pi / 2.0) # Symulacja NormalizedPitch
        
        # Clipujemy dla bezpieczeństwa do -1, 1
        error_x = np.clip(error_x, -1.0, 1.0)
        error_y = np.clip(error_y, -1.0, 1.0)
        v_x = np.clip(self.target_vel_x * 10.0, -1.0, 1.0)
        v_y = np.clip(self.target_vel_y * 10.0, -1.0, 1.0)

        return np.array([
            error_x, error_y, 
            v_x, v_y,
            self.turret_distance
        ], dtype=np.float32)

    def step(self, action):
        prev_target_x, prev_target_y = self.target_x, self.target_y

        # --- 1. RUCH CELU (Różne trajektorie) ---
        self.phase += 0.15 # 0.05
        # self.movement_type = "test" # do testowania
        if self.movement_type == "sinusoid":
            # 1. Obliczamy ruch bazowy (kierunek, w którym "pcha" move_angle)
            dir_x = np.cos(self.move_angle)
            dir_y = np.sin(self.move_angle)
            
            # 2. Obliczamy wektor prostopadły (oscylacja boczna)
            perp_x = -dir_y # Obrót o 90 stopni
            perp_y = dir_x
            
            # 3. Obliczamy pochodną sinusoidy, aby wiedzieć, jak bardzo "skręca" ścieżka
            # Używamy współczynnika, aby zrównoważyć ruch w przód i na boki
            # amplitude = np.random.uniform(2.5, 3.5) # (3.0)
            # freq = np.random.uniform(0.5, 1.5) # częstotliwość fazy (1.0)
            amplitude = 3.0 # (3.0)
            freq = 1.0 # częstotliwość fazy (1.0)
            
            # Wyznaczamy wektor przesunięcia
            # Składowa 'w przód' i składowa 'na bok' (sinus)
            move_vec_x = dir_x + perp_x * np.cos(self.phase) * amplitude * freq
            move_vec_y = dir_y + perp_y * np.cos(self.phase) * amplitude * freq
            
            # 4. NORMALIZACJA: To sprawia, że prędkość jest stała!
            length = np.sqrt(move_vec_x**2 + move_vec_y**2)
            move_vec_x = (move_vec_x / length) * self.target_speed
            move_vec_y = (move_vec_y / length) * self.target_speed
            
            self.target_x = self.target_x + move_vec_x
            self.target_y = self.target_y + move_vec_y
        elif self.movement_type == "linear":
            self.target_x += np.cos(self.move_angle) * self.target_speed
            self.target_y += np.sin(self.move_angle) * self.target_speed

        # --- ODBIJANIE OD ŚCIAN (Wymuszony powrót) ---
        if abs(self.target_x) > 1.0:
            self.move_angle = np.pi - self.move_angle
            self.phase += np.pi  # Odwrócenie fazy sinusoidy przy odbiciu
            self.target_x = np.clip(self.target_x, -1.0, 1.0)
            
        if abs(self.target_y) > 1.0:
            self.move_angle = -self.move_angle
            self.phase += np.pi
            self.target_y = np.clip(self.target_y, -1.0, 1.0)

        # Obliczanie aktualnej prędkości celu dla obserwacji
        self.target_vel_x = self.target_x - prev_target_x
        self.target_vel_y = self.target_y - prev_target_y

        # --- 2. RUCH WIEŻYCZKI (Akcja) ---
        rotateLR, rotateUD = action
        self.turret_x += float(rotateLR * self.correction_strength)
        self.turret_y += float(rotateUD * self.correction_strength)
        
        self.turret_x = np.clip(self.turret_x, -1, 1)
        self.turret_y = np.clip(self.turret_y, -1, 1)

        # --- 3. NAGRODA ---
        distance = np.linalg.norm([self.target_x - self.turret_x, self.target_y - self.turret_y])
        
        # # Kara bazowa za dystans
        # reward = -(distance ** 2) * 5.0

        # # Nagroda za "wyprzedzanie" (Lead Compensation)
        # # Model dostaje bonus, jeśli turret_x porusza się w stronę target_x
        # reward += 0.01 / (distance + 0.001)

        # # Kara za oscylacje (nadmierne ruchy)
        # reward -= 0.001 * np.linalg.norm(action)

        #####################################
        # Inny system nagrod
        #####################################
        # 1. Kara za dystans (bazowa)
        reward = -distance * 3.0

        # 2. BONUS ZA LEAD (Wyprzedzanie):
        # Sprawdzamy, czy wieżyczka celuje tam, gdzie cel BĘDZIE za chwilę (np. za 2 kroki)
        self.turret_distance -= (1.0/self.max_steps) * ((self.target_vel_x * 10.0)**2 + (self.target_vel_y*10.0)**2) * 1.2
        self.turret_distance = np.clip(self.turret_distance, 0.0, 1.0)
        future_target_x = self.target_x + self.target_vel_x * self.turret_distance * 0.005 # 0.001
        future_target_y = self.target_y + self.target_vel_y * self.turret_distance * 0.005 # 0.001

        dist_to_future = np.linalg.norm([future_target_x - self.turret_x, future_target_y - self.turret_y])
        # Jeśli dystans do "przyszłego celu" jest mniejszy niż do obecnego, dajemy bonus
        reward += (distance - dist_to_future) * 5.0

        # --- 3. KLUCZOWA ZMIANA: KARA ZA CELOWANIE Z BOKU/TYŁU ---
        # Wektor ruchu celu
        target_dir = np.array([self.target_vel_x, self.target_vel_y])
        speed = np.linalg.norm(target_dir)
        
        if speed > 0.001: # Tylko jeśli cel się porusza
            target_dir /= speed
            
            # Wektor od wieżyczki do przyszłego celu
            to_future = np.array([future_target_x - self.turret_x, future_target_y - self.turret_y])
            to_future /= np.linalg.norm(to_future)
            
            # Kąt między wektorem patrzenia a wektorem ruchu (cosinus kąta)
            # Im mniejszy cosinus, tym gorszy kąt (celujemy z boku)
            alignment = np.dot(to_future, -target_dir)
            
            # Kara: jeśli alignment jest mały (kąt > 90 stopni), kara rośnie
            # Używamy tylko części ujemnej, aby karać za bycie "z tyłu"
            self.alignment = alignment
            if alignment < 0.8: # 0.5 to około 60 stopni
                reward -= (0.8 - alignment) * 0.1
 
        # Kara za oscylacje (nadmierne ruchy)
        reward -= 0.001 * np.linalg.norm(action)

        self.step_count += 1

        # Warunek stopu = trafiam przez 20 stepow
        # hit_threshold = 0.04  # Promień trafienia
        # is_hitting = dist_to_future < hit_threshold
        # if is_hitting:
        #     self.hitting_counter += 1
        #     if self.hitting_counter >= 5:
        #         #print('hitting')
        #         reward += 10
        #         if self.alignment >= 0.8:
        #             reward += 40
        #             #print('hittinggggggggggggg')
        # else:
        #     self.hitting_counter = 0

        # print(dist_to_future)
        # if self.hitting_counter >= 3: # wypisuj co 50 kroków podczas treningu
        #     print(f"Step: {self.step_count} | Hits: {self.hitting_counter} | Dist: {dist_to_future:.4f}")

        #terminated = self.step_count >= 500 or self.hitting_counter >= 5
        terminated = self.step_count >= self.max_steps

        return self._get_obs(), reward, terminated, False, {}

    def render(self):
        pass

    def close(self):
        pass

