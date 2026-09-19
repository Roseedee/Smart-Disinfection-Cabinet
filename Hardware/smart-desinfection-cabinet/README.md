# Smart Disinfection Cabinet - Refactored

## Main change
Firebase communication has been moved to a dedicated ESP32 FreeRTOS background task.
The Arduino `loop()` never calls a blocking Firebase API.

This prevents slow Firebase responses from freezing:
- START/STOP buttons
- timer countdown
- 7-segment display
- buzzer
- door safety
- relay/motor control
- status LEDs

## Preserved features
- 4 UV lamps + motor control
- physical front-panel timer: SET / UP / DOWN / START-STOP
- Firebase task control: pending / accepted / start / stop / cancel
- task progress and remaining time synchronization
- sensor telemetry: UV raw, UV voltage, DHT11 temperature/humidity, door state
- `lastseen` update every 5 seconds with sensor telemetry in the same Firebase request
- door state immediate update when it changes
- hardware state update when lamp/motor/busy changes
- retry of failed Firebase writes without blocking the front panel
- Firebase task polling every 1 second in the background
- door-open safety pause and automatic resume for a task paused by the door
- STOP/CLEAR/FINISH cancel door-auto-resume
- WiFi initial timeout/offline mode and background reconnect after a successful connection
- existing Firebase task JSON structure and command names

## Firebase task JSON
```json
{
  "duration": 60,
  "remaining": 60,
  "progress": 0,
  "status": "pending",
  "isRunning": false,
  "command": "none",
  "lamps": {
    "L1": true,
    "L2": false,
    "L3": true,
    "L4": false
  }
}
```

Commands:
- `start`
- `stop`
- `cancel`
- `none`

## Door wiring
GPIO34 is input-only and requires an external pull-up:
- 10K resistor from GPIO34 to 3.3V
- door switch from GPIO34 to GND
- LOW = closed
- HIGH = open

## Important architecture
The main loop owns hardware/UI state.
The Firebase worker owns all Firebase network calls.
They communicate through small snapshots and pending flags protected by a critical section.

A slow Firebase request may delay Firebase synchronization, but it cannot pause the front panel loop.

## Required Arduino libraries
- Firebase ESP Client (`Firebase_ESP_Client`)
- DHT sensor library
- TM1637Display

## Credentials
`FirebaseConfig.h` contains the current project credentials supplied with the original project.
Do not commit this file to a public Git repository.
Use `FirebaseConfig.example.h` as the safe template.
