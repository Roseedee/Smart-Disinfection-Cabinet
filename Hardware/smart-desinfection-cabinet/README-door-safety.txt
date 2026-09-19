Smart Disinfection Cabinet - Door Safety

Wiring:
- GPIO34 is INPUT ONLY.
- External 10K pull-up from GPIO34 to 3.3V.
- Door switch from GPIO34 to GND.
- DOOR CLOSED = LOW.
- DOOR OPEN = HIGH.

Safety behavior:
1. Door open blocks new START commands.
2. TaskManager is the final hardware safety gate.
3. A RUNNING task is paused immediately when the door opens.
4. Relays and motor are turned OFF immediately on door open.
5. The physical timer and Firebase task timer both freeze while the door is open.
6. A task paused by the door resumes automatically when the door closes.
7. STOP/CLEAR/FINISH cancel door-auto-resume.
8. Firebase telemetry is not allowed to block this safety loop.
