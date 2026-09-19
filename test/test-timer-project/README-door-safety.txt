Smart Disinfection Cabinet - Door Safety Patch

Files in this patch:
- Smart-Disinfection-Cabinet.ino
- SensorManager.h
- SensorManager.cpp
- Timer.h
- Timer.cpp
- TaskManager.h
- TaskManager.cpp
- FirebaseManager.h
- FirebaseManager.cpp

Door wiring required:
- GPIO34 is INPUT ONLY.
- External 10K pull-up from GPIO34 to 3.3V.
- Door switch from GPIO34 to GND.
- DOOR CLOSED = LOW.
- DOOR OPEN   = HIGH.

Safety behavior:
1. Door open blocks new START from the physical timer path.
2. TaskManager is the final hardware safety gate; submit() rejects new tasks while door is open.
3. If a task is already RUNNING and the door opens, relays/motor are turned OFF and task becomes PAUSED.
4. Timer freezes at the current remaining time while door is open.
5. When the door closes, a task paused by the door resumes automatically.
6. STOP/CLEAR/FINISH cancel the door-auto-resume state.
7. Existing FirebaseManager interface is the 5-argument version that accepts SensorManager&.
