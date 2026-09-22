package com.app.sdc

class DeviceStatusManager {
    var deviceSn: String = ""
        private set

    var isOnline: Boolean = false
        private set

    var lastSeen: Long = 0
        private set

    fun update(
        sn: String,
        lastSeen: Long
    ) {
        this.deviceSn = sn
        this.lastSeen = lastSeen

        val now = System.currentTimeMillis() / 1000

        isOnline = (now - lastSeen) <= 15
    }
}