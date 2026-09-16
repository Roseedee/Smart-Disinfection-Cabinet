task form

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

command = start || stop || cancel || none

app send command(start) -> firebase -> esp get command -> command(none) to firebase