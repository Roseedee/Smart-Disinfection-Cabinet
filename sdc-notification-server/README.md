## About
### Node.js Entry Point
```
index.js
```
### Firebase Credential
```
serviceAccountKey.json
```
<small>Never commit ```serviceAccountKey.json``` to Git.</small>

---
## Service Control
### Start
``` 
sudo systemctl start sdc-notification 
```
### Stop
```
sudo systemctl stop sdc-notification 
```
### Restart
```
sudo systemctl restart sdc-notification
```
### Status
```
sudo systemctl status sdc-notification
```
### Follow Logs in Realtime
```
journalctl -u sdc-notification.service -f
```