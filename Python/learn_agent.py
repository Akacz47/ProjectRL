import torch
from stable_baselines3 import SAC
from stable_baselines3.common.env_checker import check_env
from UE5TurretEnvSimple import UE5TurretEnvSimple
from UE5TurretEnvUDP import UE5TurretEnvUDP

MODEL_TYPE = 'SIMPLE' # UDP or SIMPLE

if MODEL_TYPE == 'UDP':
    env = UE5TurretEnvUDP()
    gamma = 0.99 # 0.99
    batch_size = 1024 # 1024
    learning_starts = 2000 # 2000
    policy_kwargs = dict(net_arch=dict(pi=[400, 300], qf=[400, 300])) # 400 300
elif MODEL_TYPE == 'SIMPLE':
    env = UE5TurretEnvSimple()
    gamma = 0.98 # 0.98
    batch_size = 256 # 256
    learning_starts = 500 # 500
    policy_kwargs = dict(net_arch=dict(pi=[256, 256], qf=[256, 256])) # 256 256
check_env(env, warn=True)

device = "cuda" if torch.cuda.is_available() else "cpu"
print("Using device:", device)

# model = SAC(
#     "MlpPolicy",
#     env,
#     policy_kwargs=policy_kwargs,
#     verbose=1,
#     device=device,
#     gamma=gamma,
#     batch_size=batch_size,
#     learning_starts=learning_starts,
#     tensorboard_log="./sac_turret_tensorboard"
# )
# model.learn(total_timesteps=150000)
# if MODEL_TYPE == 'UDP':
#     model.save("sac_turret_model_udp")
# elif MODEL_TYPE == 'SIMPLE':
#     model.save("sac_turret_model_simple")
# env.close()

# SIMPLE:
# Gamma=0.98
# batch_size=256
# learning_starts=500
# policy_kwargs = dict(net_arch=dict(pi=[256, 256], qf=[256, 256]))

# UDP:
# Gamma: 0.99
# batch_size=1024
# learning_starts=2000
# 100k
# policy_kwargs = dict(net_arch=dict(pi=[400, 300], qf=[400, 300]))




model = SAC.load("sac_model_simple_deep_g098_b256", env=env, device=device)
model.learn(total_timesteps=50000, reset_num_timesteps=False)
model.save("sac_model_simple_deep_g098_b256_150k")
env.close()