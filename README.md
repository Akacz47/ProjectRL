# ProjectRL

[English](#english) | [Polski](#polski)

## English

A 3D game built with Unreal Engine 5 in which the player competes against an autonomous turret controlled by a reinforcement learning model. Both turrets aim at moving targets, taking target velocity and projectile travel time into account.

The project includes game mechanics, two Python training environments, training with **Soft Actor-Critic (SAC)**, and network export to **ONNX**. The trained model runs directly in the engine through ONNX Runtime, so gameplay does not require running Python scripts.

### Gameplay

- The player and the AI agent control separate turrets that rotate along the yaw and pitch axes.
- Enemies follow a sinusoidal path toward the finish line. A hit removes the enemy and awards a point to the turret that fired the projectile.
- Each round consists of a wave of five enemies spawning at two-second intervals. The turret with the higher score wins.
- Reloading takes one second. The agent fires whenever it can, while the player chooses when to shoot.
- The game has no crosshair, making manual aiming and estimating lead more challenging.

### Technologies

| Area | Technology |
| --- | --- |
| Game engine | Unreal Engine 5.7, C++, and Blueprints |
| AI logic | AI Controller, AI Perception, Behavior Tree, and Blackboard |
| Reinforcement learning | SAC from Stable-Baselines3 |
| Training environments | Python, Gymnasium, and NumPy |
| Neural networks and export | PyTorch and ONNX |
| In-game inference | ONNX Runtime, C++ API |
| Training communication | UDP, binary data packets |
| Diagnostics | TensorBoard and Matplotlib |

### Architecture

Preparing the agent and running it in the game involves three stages:

1. **Training in Python** - the SAC agent collects experience in the selected Gymnasium environment and learns to control the turret using a reward function.
2. **Actor export** - the deterministic part of the network responsible for selecting actions is saved in ONNX format. The `tanh` function constrains the output to `[-1, 1]`.
3. **Inference in UE5** - the `BTT_Predict` task processes observations through ONNX Runtime and writes the resulting rotation to the Blackboard. Behavior Tree tasks handle rotation and firing.

The perception system detects enemies and provides a target to the AI controller. The model computes control actions from five input values:

| Observation | Meaning | Range |
| --- | --- | --- |
| `error_x` | Normalized aiming error along the yaw axis | `[-1, 1]` |
| `error_y` | Normalized aiming error along the pitch axis | `[-1, 1]` |
| `target_vel_x` | Normalized X component of target velocity | `[-1, 1]` |
| `target_vel_y` | Normalized Y component of target velocity | `[-1, 1]` |
| `turret_distance` | Normalized distance between the turret and the target | `[0, 1]` |

The model outputs two continuous values in `[-1, 1]`, controlling horizontal and vertical rotation. The decision to fire is handled by the game logic.

#### Two training approaches

**SIMPLE - simplified simulation in Python**

The `UE5TurretEnvSimple` environment simulates a target moving along a straight or sinusoidal path. It randomizes initial conditions and supports training without launching the game engine. The reward function encourages target tracking and leading the target while discouraging unwanted agent behavior. Removing communication with an external process speeds up experience collection, but the simplified dynamics do not fully reproduce UE5 physics.

**UDP - training with Unreal Engine**

The `UE5TurretEnvUDP` environment exchanges data with the running game at `127.0.0.1`. UE5 receives actions on port `9000`, while Python receives the game state on port `8000`. Each state packet contains six `float32` values: five observations and the projectile-to-target distance used to calculate the reward. Each action packet contains two `float32` values defining turret rotation.

In this approach, the reward accounts for aiming error, shot precision, time to hit, and a penalty for excessive movement. The training loop depends on synchronization between Python and the game engine.

### Experimental results

Models were evaluated after deployment in UE5 using a scenario with **25 enemies**: five waves of five targets, with the player not firing. The success rate measures the proportion of enemies eliminated out of all targets, rather than the proportion of projectiles that hit.

The SIMPLE approach was compared across two network architectures, `standard` (`[256, 256]`) and `deep` (`[400, 300]`), discount factors of `0.95` and `0.98`, and batch sizes of `256` and `512`.

| SIMPLE configuration | Training steps | Success rate in UE5 |
| --- | --- | --- |
| `standard_g095_b256` | 100,000 | 32% |
| `standard_g095_b512` | 100,000 | 44% |
| `standard_g098_b256` | 100,000 | 8% |
| `standard_g098_b512` | 100,000 | 8% |
| `deep_g095_b256` | 100,000 | 0% |
| `deep_g095_b512` | 100,000 | 40% |
| `deep_g098_b256` | 100,000 | 56% |
| `deep_g098_b512` | 100,000 | 12% |
| **`standard_g098_b256` with extended training** | **150,000** | **76%** |

The best configuration was selected for the game. Additional training did not always improve results: continuing training of `deep_g098_b256` to 150,000 steps reduced its success rate to 0%.

All four tested UDP configurations achieved a **0% success rate after export and deployment in the game**, despite signs of improvement during training. Suspected causes included latency and differences in action execution frequency between training and inference. Increasing data exchange throughput to approximately 80-90 steps per second did not resolve the issue.

These results apply to the specific test scenario. The small number of targets, training randomness, and differences between the simulation and the engine limit how broadly they can be generalized. Higher training rewards do not guarantee better in-game performance.

### Repository structure

```text
ProjectRL.uproject              Unreal Engine project
Source/ProjectRL/
  AIC_Turret.*                 AI controller and UE5 training support
  BTT_Predict.*                ONNX inference in the Behavior Tree
  Bullet.*                     Projectile logic
  UDPSubsystem.*               Binary communication over UDP
  ProjectRL.Build.cs           Modules and libraries required for compilation
Content/
  Characters/                 Turret, enemy, and projectile Blueprints
  Models/                     ONNX models
  ThirdPerson/                Map and gameplay configuration
  UI/                         Score interface
Python/
  UE5TurretEnvSimple.py        Simplified Gymnasium environment
  UE5TurretEnvUDP.py           Gymnasium environment connected to UE5
  unreal_socket_client.py      UDP packet serialization and exchange
  learn_agent.py              SAC training and continued learning
  export_model.py             Actor export to ONNX
  test_model.py               Behavior visualization in the SIMPLE simulation
  sac_turret_tensorboard/      Training logs
Models_zip/                   Saved Stable-Baselines3 models
ThirdParty.zip                ONNX Runtime dependency archive
```

### Running the game

The current C++ configuration targets **Windows x64**.

1. Set up Unreal Engine **5.7** and Visual Studio with C++ tools for Unreal Engine and the Windows SDK.
2. If the `ThirdParty/onnxruntime` directory is missing, extract `ThirdParty.zip` into the project root. `ProjectRL.Build.cs` expects headers in `ThirdParty/onnxruntime/include` and the `onnxruntime.lib` and `onnxruntime.dll` files in `ThirdParty/onnxruntime/lib`.
3. Adjust the paths in the code to match your local project and engine locations:
   - In `Source/ProjectRL/BTT_Predict.cpp`, set the model path in `InitializeONNX()`. The default `SIMPLE` variant loads `Content/Models/sac_actor_simple.onnx`.
   - In `Source/ProjectRL/BTT_Predict.h`, check the path to `onnxruntime_cxx_api.h`, which currently refers to a specific UE 5.7 installation directory layout.
4. Generate Visual Studio project files from `ProjectRL.uproject`, then build the `Development Editor` configuration for `Win64`.
5. Open `ProjectRL.uproject`. The default map is `Content/ThirdPerson/Lvl_ThirdPerson.umap`.
6. Start gameplay using the editor's **Play** button.

Model selection is controlled by the `loaded_model` field in `BTT_Predict.h`, which defaults to `SIMPLE`. The SIMPLE and UDP variants process target velocity differently, so observations must remain consistent with the model's training environment when switching models.

### Training, diagnostics, and export

The scripts use `torch`, `stable-baselines3`, `gymnasium`, `numpy`, `onnx`, `tensorboard`, and `matplotlib`. Visualization in `test_model.py` also requires Tkinter support because it uses the `TkAgg` backend.

Example environment setup in PowerShell, starting from the project root:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install torch stable-baselines3 gymnasium numpy onnx tensorboard matplotlib
cd Python
```

The repository does not include a file with pinned Python dependency versions.

Adjust the scripts' configuration before running them:

- **`learn_agent.py`**: select `MODEL_TYPE` (`SIMPLE` or `UDP`). The currently active code loads `sac_model_simple_deep_g098_b256` and continues training for 50,000 steps. To start a new training run, use the provided, commented-out block that creates `SAC`, and set the parameters and output name. Input models are located in `Models_zip/`, so provide the appropriate path or copy the selected file to `Python/`.
- **`test_model.py`**: set the path in `SAC.load(...)`. The script visualizes the target trajectory and observations for the SIMPLE environment only.
- **`export_model.py`**: select `MODEL_TYPE` and the model path in `SAC.load(...)`. The script exports the actor with an `obs` input of shape `[1, 5]` and an `action` output of shape `[1, 2]`, using ONNX opset 15.

After configuring the scripts, run the desired stage from the `Python/` directory:

```powershell
python learn_agent.py
python test_model.py
python export_model.py
```

Copy the exported model to `Content/Models/` using the filename expected by `InitializeONNX()`. Training logs included in the repository can be viewed with:

```powershell
tensorboard --logdir sac_turret_tensorboard
```

For UDP training, also run the training scenario in UE5 with action handling and state transmission enabled in the turret controller. Python waits for packets from the game; launching the script alone without active data exchange will not start training.

---

## Polski

Gra 3D w Unreal Engine 5, w której gracz rywalizuje z autonomiczną wieżyczką sterowaną przez model uczenia ze wzmocnieniem. Zadaniem obu wieżyczek jest trafianie w poruszające się cele, z uwzględnieniem ich prędkości i czasu dolotu pocisku.

Projekt obejmuje mechanikę gry, dwa środowiska treningowe w Pythonie, trening algorytmem **Soft Actor-Critic (SAC)** oraz eksport sieci do **ONNX**. Wytrenowany model działa bezpośrednio w silniku przez ONNX Runtime, więc rozgrywka nie wymaga uruchamiania skryptów Pythona.

### Mechanika gry

- Gracz i agent AI sterują osobnymi wieżyczkami obracającymi się w osiach yaw i pitch.
- Przeciwnicy poruszają się po sinusoidalnej trajektorii w kierunku mety. Trafiony przeciwnik znika, a strzelająca wieżyczka otrzymuje punkt.
- Runda obejmuje falę pięciu przeciwników pojawiających się co dwie sekundy. Wygrywa wieżyczka z większą liczbą punktów.
- Przeładowanie trwa jedną sekundę. Agent strzela przy każdej dostępnej okazji, a gracz wybiera moment strzału.
- Gra nie udostępnia celownika, co utrudnia ręczne celowanie i ocenę wyprzedzenia.

### Technologie

| Obszar | Rozwiązanie |
| --- | --- |
| Silnik gry | Unreal Engine 5.7, C++ i Blueprints |
| Logika AI | AI Controller, AI Perception, Behavior Tree i Blackboard |
| Uczenie ze wzmocnieniem | SAC z biblioteki Stable-Baselines3 |
| Środowiska treningowe | Python, Gymnasium i NumPy |
| Sieci neuronowe i eksport | PyTorch i ONNX |
| Inferencja w grze | ONNX Runtime, interfejs C++ |
| Komunikacja treningowa | UDP, binarne pakiety danych |
| Diagnostyka | TensorBoard i Matplotlib |

### Architektura

Proces przygotowania agenta i jego działania w grze składa się z trzech etapów:

1. **Trening w Pythonie** - agent SAC zbiera doświadczenia w wybranym środowisku Gymnasium i uczy się sterowania wieżyczką na podstawie funkcji nagrody.
2. **Eksport aktora** - deterministyczna część sieci odpowiedzialna za wybór akcji jest zapisywana w formacie ONNX. Funkcja `tanh` ogranicza wyjście do zakresu `[-1, 1]`.
3. **Inferencja w UE5** - zadanie `BTT_Predict` przetwarza obserwacje przez ONNX Runtime i zapisuje wynikową rotację w Blackboard. Zadania drzewa zachowań realizują obrót i strzał.

System percepcji wykrywa przeciwnika i udostępnia cel kontrolerowi AI. Model wyznacza sterowanie na podstawie pięciu wartości wejściowych:

| Obserwacja | Znaczenie | Zakres |
| --- | --- | --- |
| `error_x` | Znormalizowany błąd celowania w osi yaw | `[-1, 1]` |
| `error_y` | Znormalizowany błąd celowania w osi pitch | `[-1, 1]` |
| `target_vel_x` | Znormalizowana składowa prędkości celu w osi X | `[-1, 1]` |
| `target_vel_y` | Znormalizowana składowa prędkości celu w osi Y | `[-1, 1]` |
| `turret_distance` | Znormalizowana odległość wieżyczki od celu | `[0, 1]` |

Wyjściem modelu są dwie ciągłe wartości w zakresie `[-1, 1]`, sterujące obrotem poziomym i pionowym. Decyzja o oddaniu strzału jest realizowana przez logikę gry.

#### Dwa warianty treningu

**SIMPLE - uproszczona symulacja w Pythonie**

Środowisko `UE5TurretEnvSimple` symuluje cel poruszający się liniowo lub sinusoidalnie. Losuje warunki początkowe i pozwala trenować bez uruchamiania silnika gry. Funkcja nagrody promuje śledzenie celu i celowanie z wyprzedzeniem oraz ogranicza niepożądane zachowania agenta. Brak komunikacji z zewnętrznym procesem przyspiesza zbieranie doświadczeń, ale uproszczona dynamika nie odwzorowuje w pełni fizyki UE5.

**UDP - trening z udziałem Unreal Engine**

Środowisko `UE5TurretEnvUDP` wymienia dane z uruchomioną grą na adresie `127.0.0.1`. UE5 odbiera akcje na porcie `9000`, a Python odbiera stan gry na porcie `8000`. Pakiet stanu zawiera sześć liczb `float32`: pięć obserwacji oraz odległość pocisku od celu wykorzystywaną do obliczania nagrody. Pakiet akcji zawiera dwie liczby `float32` określające obrót wieżyczki.

W tym wariancie nagroda uwzględnia błąd celowania, precyzję strzału, szybkość trafienia i karę za nadmierne ruchy. Pętla treningowa zależy od synchronizacji Pythona z silnikiem gry.

### Wyniki eksperymentów

Modele oceniano po przeniesieniu do UE5, w scenariuszu obejmującym **25 przeciwników**: pięć fal po pięć celów, bez strzelania przez gracza. Skuteczność oznacza udział wyeliminowanych przeciwników w całkowitej liczbie celów, a nie odsetek trafionych pocisków.

Wariant SIMPLE porównano dla dwóch architektur sieci: `standard` (`[256, 256]`) i `deep` (`[400, 300]`), współczynników dyskontowania `0.95` i `0.98` oraz rozmiarów paczki `256` i `512`.

| Konfiguracja SIMPLE | Kroki treningu | Skuteczność w UE5 |
| --- | --- | --- |
| `standard_g095_b256` | 100 000 | 32% |
| `standard_g095_b512` | 100 000 | 44% |
| `standard_g098_b256` | 100 000 | 8% |
| `standard_g098_b512` | 100 000 | 8% |
| `deep_g095_b256` | 100 000 | 0% |
| `deep_g095_b512` | 100 000 | 40% |
| `deep_g098_b256` | 100 000 | 56% |
| `deep_g098_b512` | 100 000 | 12% |
| **`standard_g098_b256` po wydłużeniu treningu** | **150 000** | **76%** |

Najlepszą konfigurację wybrano do gry. Dalsze uczenie nie zawsze poprawiało wyniki: dotrenowanie wariantu `deep_g098_b256` do 150 000 kroków obniżyło skuteczność do 0%.

Wszystkie cztery testowane konfiguracje UDP uzyskały **0% skuteczności po eksporcie i uruchomieniu w grze**, mimo oznak poprawy podczas treningu. Podejrzewaną przyczyną były opóźnienia oraz różnice w częstotliwości wykonywania akcji między treningiem a inferencją. Przyspieszenie wymiany danych do około 80-90 kroków na sekundę nie rozwiązało problemu.

Wyniki dotyczą konkretnego scenariusza testowego. Niewielka liczba celów, losowość treningu oraz różnice między symulacją a silnikiem ograniczają możliwość ich uogólniania. Wzrost nagrody treningowej nie gwarantuje wyższej skuteczności w grze.

### Struktura repozytorium

```text
ProjectRL.uproject              Projekt Unreal Engine
Source/ProjectRL/
  AIC_Turret.*                 Kontroler AI i obsługa treningu z UE5
  BTT_Predict.*                Inferencja ONNX w drzewie zachowań
  Bullet.*                     Logika pocisku
  UDPSubsystem.*               Komunikacja binarna przez UDP
  ProjectRL.Build.cs           Moduły i biblioteki wymagane do kompilacji
Content/
  Characters/                 Blueprints wieżyczki, przeciwnika i pocisku
  Models/                     Modele ONNX
  ThirdPerson/                Mapa i konfiguracja rozgrywki
  UI/                         Interfejs punktacji
Python/
  UE5TurretEnvSimple.py        Uproszczone środowisko Gymnasium
  UE5TurretEnvUDP.py           Środowisko Gymnasium połączone z UE5
  unreal_socket_client.py      Serializacja i wymiana pakietów UDP
  learn_agent.py              Trening i kontynuowanie uczenia SAC
  export_model.py             Eksport aktora do ONNX
  test_model.py               Wizualizacja zachowania w symulacji SIMPLE
  sac_turret_tensorboard/      Logi treningowe
Models_zip/                   Zapisane modele Stable-Baselines3
ThirdParty.zip                Archiwum zależności ONNX Runtime
```

### Uruchomienie gry

Obecna konfiguracja kodu C++ jest przygotowana dla **Windows x64**.

1. Przygotuj Unreal Engine **5.7** oraz Visual Studio z narzędziami C++ dla Unreal Engine i Windows SDK.
2. Jeśli brakuje katalogu `ThirdParty/onnxruntime`, rozpakuj `ThirdParty.zip` w katalogu głównym projektu. Plik `ProjectRL.Build.cs` oczekuje nagłówków w `ThirdParty/onnxruntime/include` oraz plików `onnxruntime.lib` i `onnxruntime.dll` w `ThirdParty/onnxruntime/lib`.
3. Dostosuj ścieżki w kodzie do lokalnego położenia projektu i silnika:
   - W `Source/ProjectRL/BTT_Predict.cpp`, w metodzie `InitializeONNX()`, ustaw ścieżkę do modelu. Domyślny wariant `SIMPLE` ładuje `Content/Models/sac_actor_simple.onnx`.
   - W `Source/ProjectRL/BTT_Predict.h` sprawdź ścieżkę nagłówka `onnxruntime_cxx_api.h`, która obecnie odwołuje się do konkretnego układu katalogów instalacji UE 5.7.
4. Wygeneruj pliki projektu Visual Studio z pliku `ProjectRL.uproject`, a następnie skompiluj konfigurację `Development Editor` dla `Win64`.
5. Otwórz `ProjectRL.uproject`. Domyślną mapą jest `Content/ThirdPerson/Lvl_ThirdPerson.umap`.
6. Uruchom rozgrywkę przyciskiem **Play** w edytorze.

Wybór modelu kontroluje pole `loaded_model` w `BTT_Predict.h`, domyślnie ustawione na `SIMPLE`. Warianty SIMPLE i UDP stosują inne przetwarzanie prędkości celu, dlatego przy zmianie modelu należy zachować zgodność obserwacji z jego środowiskiem treningowym.

### Trening, diagnostyka i eksport

Skrypty korzystają z bibliotek `torch`, `stable-baselines3`, `gymnasium`, `numpy`, `onnx`, `tensorboard` i `matplotlib`. Wizualizacja w `test_model.py` wymaga także obsługi Tkinter, ponieważ używa backendu `TkAgg`.

Przykładowe przygotowanie środowiska w PowerShell, z katalogu głównego projektu:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install torch stable-baselines3 gymnasium numpy onnx tensorboard matplotlib
cd Python
```

Repozytorium nie zawiera pliku z przypiętymi wersjami zależności Pythona.

Przed uruchomieniem skryptów dostosuj ich konfigurację:

- **`learn_agent.py`**: wybierz `MODEL_TYPE` (`SIMPLE` lub `UDP`). Aktualnie aktywny kod wczytuje `sac_model_simple_deep_g098_b256` i kontynuuje trening o 50 000 kroków. Dla nowego treningu użyj przygotowanego, zakomentowanego bloku tworzenia `SAC` oraz ustaw parametry i nazwę zapisu. Modele wejściowe znajdują się w `Models_zip/`, więc podaj odpowiednią ścieżkę lub skopiuj wybrany plik do katalogu `Python/`.
- **`test_model.py`**: ustaw ścieżkę w `SAC.load(...)`. Skrypt wizualizuje trajektorię celu i obserwacje wyłącznie dla środowiska SIMPLE.
- **`export_model.py`**: wybierz `MODEL_TYPE` i ścieżkę modelu w `SAC.load(...)`. Skrypt eksportuje aktora z wejściem `obs` o wymiarach `[1, 5]` i wyjściem `action` o wymiarach `[1, 2]`, korzystając z ONNX opset 15.

Po dostosowaniu konfiguracji uruchamiaj wybrany etap z katalogu `Python/`:

```powershell
python learn_agent.py
python test_model.py
python export_model.py
```

Wyeksportowany model skopiuj do `Content/Models/` pod nazwą oczekiwaną przez `InitializeONNX()`. Logi treningów zapisane w repozytorium można przeglądać poleceniem:

```powershell
tensorboard --logdir sac_turret_tensorboard
```

Dla treningu UDP uruchom również scenariusz treningowy w UE5 z aktywną obsługą akcji i wysyłaniem stanu przez kontroler wieżyczki. Python oczekuje na pakiety z gry; samo uruchomienie skryptu bez działającej wymiany danych nie rozpocznie treningu.
