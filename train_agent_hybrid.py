# from stable_baselines3 import PPO
from UE5TurretEnvHybrid2 import UE5TurretEnv
import torch

env = UE5TurretEnv()

device = "cuda" if torch.cuda.is_available() else "cpu"
print("Using device:", device)

print("Start...")

# Uczenie od zera
from stable_baselines3 import SAC
#policy_kwargs = dict(net_arch=[400, 300])
#policy_kwargs = dict(net_arch=[512, 512, 256])
#policy_kwargs = dict(net_arch=[512, 512, 512, 512, 512, 512, 512, 512, 512, 256])
policy_kwargs = dict(net_arch=dict(pi=[400, 300], qf=[400, 300]))
model = SAC(
    "MlpPolicy", 
    env,
    verbose=1,
    learning_starts=2000, # 1000
    batch_size=1024, # 512
    policy_kwargs=policy_kwargs,
    gamma=0.95,
    #tensorboard_log="./sac_turret_tensorboard"
)
model.learn(total_timesteps=100000)
model.save("sac_turret_model_hybrid_10w")
env.close()

# Uczenie od offline
# from stable_baselines3 import SAC
# # model = SAC.load("sac_turret_model_offline", env=env)
# model = SAC.load(
#     "sac_turret_model_hybrid", 
#     env=env,
#     custom_objects={
#         "learning_rate": 3e-5,    # Zmniejszamy tempo nauki, by nie zepsuć wag
#         "gamma": 0.95             # Lekko zwiększamy horyzont, by lepiej planował lead
#     }
# )
# model.learn(total_timesteps=100000, reset_num_timesteps=False)
# model.save("sac_turret_model_hybrid2")
# env.close()




# new_model = SAC(
#     "MlpPolicy", 
#     env,
#     verbose=1,
#     gamma=0.95,              # Lekko wyższa gamma dla lepszego planowania
#     learning_rate=3e-5,      # Mały LR, żeby nie zepsuć dotychczasowej nauki
#     ent_coef=0.05,           # KLUCZ: Stała niska entropia = brak drgań
#     policy_kwargs=dict(net_arch=[512, 512, 256]), # Architektura MUSI być ta sama
#     tensorboard_log="./sac_turret_tensorboard"
# )

# # 3. Załaduj "stary" model (ten, który drgał i miał górkę)
# old_model = SAC.load("sac_turret_model_hybrid")

# # 4. PRZESZCZEP WAG: Kopiujemy mózg ze starego do nowego
# # To kopiuje wyuczone parametry sieci Actor i Critic
# new_model.policy.load_state_dict(old_model.policy.state_dict())

# print("Przeszczep wag zakończony pomyślnie. Model nie będzie już drgać.")

# # 5. Kontynuuj naukę (dotrenowanie)
# # Nowy model od początku wie, że ma stałą entropię, więc nie wywali błędu
# new_model.learn(total_timesteps=100000, reset_num_timesteps=False)

# # 6. Zapisz nową wersję
# new_model.save("sac_turret_model_hybrid_v4_stable")
# env.close()