import torch
from stable_baselines3 import SAC

MODEL_TYPE = 'SIMPLE' # UDP or SIMPLE

if MODEL_TYPE == 'UDP':
    model = SAC.load("sac_turret_model_udp.zip", device="cpu")
    MODEL_NAME = 'sac_actor_udp.onnx'
elif MODEL_TYPE == 'SIMPLE':
    model = SAC.load("sac_turret_model_simple.zip", device="cpu")
    MODEL_NAME = 'sac_actor_simple.onnx'

class SACActorWrapper(torch.nn.Module):
    def __init__(self, actor):
        super().__init__()
        self.actor = actor

    def forward(self, obs):
        # Sieć aktora w trybie deterministycznym
        mean_actions, _, _ = self.actor.get_action_dist_params(obs)
        # SB3 nakłada Tanh na mean_actions, aby uzyskać zakres [-1, 1]
        return torch.tanh(mean_actions)

actor_wrapped = SACActorWrapper(model.policy.actor)
actor_wrapped.eval() # Ustawienie w tryb ewaluacji

obs_dim = model.observation_space.shape[0]
dummy_input = torch.randn(1, obs_dim)

# Eksport ONNX
torch.onnx.export(
    actor_wrapped,
    dummy_input,
    MODEL_NAME,
    export_params=True,
    opset_version=15,
    do_constant_folding=True,
    input_names=['obs'],
    output_names=['action'],
)
print("Model wyeksportowany")