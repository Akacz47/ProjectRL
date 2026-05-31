import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
from UE5TurretEnvSimple import UE5TurretEnvSimple
from stable_baselines3 import SAC

model = SAC.load("sac_turret_model_simple")
env = UE5TurretEnvSimple()
obs, _ = env.reset()

plt.ion()
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
t_x_hist, t_y_hist = [], []

for i in range(500):
    action, _states = model.predict(obs, deterministic=True)
    
    obs, reward, terminated, truncated, info = env.step(action)
    
    tx, ty = env.target_x, env.target_y
    ex, ey = obs[0], obs[1]
    tvx, tvy = obs[2], obs[3]
    
    t_x_hist.append(tx)
    t_y_hist.append(ty)
    
    # Lewy wykres: pole bitwy
    ax1.clear()
    ax1.set_xlim(-1.2, 1.2)
    ax1.set_ylim(-1.2, 1.2)
    ax1.set_title(f"Pole bitwy (Typ: {env.movement_type})")
    
    ax1.plot(t_x_hist, t_y_hist, 'r--', alpha=0.3, label="Trajektoria celu")
    ax1.scatter(tx, ty, c='red', s=75, label="CEL (target_x/y)")
    
    ax1.scatter(env.turret_x, env.turret_y, c='blue', marker='x', s=75, label="WIEŻYCZKA")
    ax1.legend(loc='upper right')

    # Prawy wykres: obserwacje
    ax2.clear()
    ax2.set_ylim(-1.1, 1.1)
    ax2.set_title("Wartości wektora obserwacji")
    
    labels = ['Error X', 'Error Y', 'Vel X', 'Vel Y', 'Turret Distance']
    values = [ex, ey, tvx, tvy, env.turret_distance]
    colors = ['orange', 'orange', 'green', 'green', 'yellow']
    
    bars = ax2.bar(labels, values, color=colors)
    ax2.axhline(0, color='black', linewidth=0.8)
    
    for bar in bars:
        yval = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width()/2, yval, round(yval, 3), va='bottom' if yval > 0 else 'top', ha='center')

    plt.pause(0.01)
    if not plt.fignum_exists(fig.number) or terminated: break

plt.ioff()
plt.show()
env.close()