import torch
from stable_baselines3 import SAC
from stable_baselines3.common.env_checker import check_env
from UE5TurretEnvSimple import UE5TurretEnvSimple
from UE5TurretEnvUDP import UE5TurretEnvUDP

MODEL_TYPE = 'UDP' # UDP or SIMPLE

if MODEL_TYPE == 'UDP':
    env = UE5TurretEnvUDP()
elif MODEL_TYPE == 'SIMPLE':
    env = UE5TurretEnvSimple()
check_env(env, warn=True)

device = "cuda" if torch.cuda.is_available() else "cpu"
print("Using device:", device)

policy_kwargs = dict(
    net_arch=dict(pi=[256, 256], qf=[256, 256]) #128 128
)

model = SAC(
    "MlpPolicy",
    env,
    policy_kwargs=policy_kwargs,
    verbose=1,
    device=device,
    gamma=0.98,
    learning_starts=500,
    tensorboard_log="./sac_turret_tensorboard"
)
model.learn(total_timesteps=100000)
if MODEL_TYPE == 'UDP':
    model.save("sac_turret_model_udp")
elif MODEL_TYPE == 'SIMPLE':
    model.save("sac_turret_model_simple")
env.close()

# SIMPLE:
# Gamma: 0.98
# learning_starts: 500
# policy_kwargs = dict(
#    net_arch=dict(pi=[256, 256], qf=[256, 256]) #128 128
# )
# 

# UDP:
# Gamma: 0.99
# batch_size=1024
# learning_starts=2000
# 100k
# policy_kwargs = dict(net_arch=dict(pi=[400, 300], qf=[400, 300]))