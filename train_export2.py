import torch
from stable_baselines3 import SAC

# 1. Wczytaj model
#model = SAC.load("sac_turret_model_offline.zip", device="cpu")
model = SAC.load("sac_turret_model_hybrid2.zip", device="cpu")

# 2. Definicja Wrappera - to rozwiązuje TracerWarning i błędy logiki
# Wrapper wymusza tryb deterministyczny i nakłada Tanh na wyjście
class SACActorWrapper(torch.nn.Module):
    def __init__(self, actor):
        super().__init__()
        self.actor = actor

    def forward(self, obs):
        # Wywołujemy sieć aktora w trybie deterministycznym
        # To kluczowe, aby model nie "losował" akcji w Unrealu
        mean_actions, _, _ = self.actor.get_action_dist_params(obs)
        # SB3 nakłada Tanh na mean_actions, aby uzyskać zakres [-1, 1]
        return torch.tanh(mean_actions)

# 3. Przygotowanie komponentów
actor_wrapped = SACActorWrapper(model.policy.actor)
actor_wrapped.eval() # Ustawienie w tryb ewaluacji

obs_dim = model.observation_space.shape[0]
dummy_input = torch.randn(1, obs_dim)

# 4. Eksport ONNX
torch.onnx.export(
    actor_wrapped,
    dummy_input,
    "sac_actor2.onnx",
    export_params=True,
    opset_version=15, # Wersja 15 jest bardzo stabilna dla UE5
    do_constant_folding=True,
    input_names=['obs'],
    output_names=['action'],
    # Usunąłem dynamic_axes, bo Unreal NNE czasem ma problem z dynamicznym batchem
    # Stały rozmiar [1, obs_dim] jest bezpieczniejszy dla zadań czasu rzeczywistego
)

print("Sukces! Model wyeksportowany")