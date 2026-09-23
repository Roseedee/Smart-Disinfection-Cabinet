package com.app.sdc.view

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.widget.FrameLayout
import android.widget.ProgressBar
import android.widget.TextView
import com.app.sdc.R
import com.app.sdc.UserSession
import com.google.firebase.FirebaseApp
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener

class DashboardView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    private var dashboardRef: DatabaseReference? = null
    private var dashboardListener: ValueEventListener? = null
    private var devicesn: String

    var onAddTaskClick: (() -> Unit)? = null

    init {
        LayoutInflater.from(context).inflate(
            R.layout.view_dashboard,
            this,
            true
        )

        devicesn = UserSession.getDeviceSN(context) ?: ""

        observeDashboard()

        findViewById<TextView>(R.id.openAddTaskView).setOnClickListener {
            onAddTaskClick?.invoke()
        }
    }

    private fun observeDashboard() {
        if (FirebaseApp.getApps(context).isEmpty()) {
            return
        }

        dashboardRef = FirebaseDatabase.getInstance()
            .getReference("devices")
            .child(devicesn)
            .child("hw_status")

        dashboardListener = object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                renderDashboard(snapshot)
            }

            override fun onCancelled(error: DatabaseError) {
                findViewById<TextView>(R.id.usageStatusText).text = "เกิดข้อผิดพลาด"
            }
        }

        dashboardRef?.addValueEventListener(dashboardListener as ValueEventListener)
    }

    private fun renderDashboard(snapshot: DataSnapshot) {
        val door_open = snapshot.boolean("sensors/door_open", false)
        val temperatureCurrent = snapshot.double("sensors/temperature", 0.0)
        val humidityCurrent = snapshot.double("sensors/humidity", 0.0)
        val uv_raw = snapshot.double("sensors/uv_raw", 0.0)
        val lamp1 = snapshot.boolean("lamps/1", false)
        val lamp2 = snapshot.boolean("lamps/2", false)
        val lamp3 = snapshot.boolean("lamps/3", false)
        val lamp4 = snapshot.boolean("lamps/4", false)
        val motor_status = snapshot.boolean("motor", false)

        findViewById<TextView>(R.id.currentTemperatureText).text = "ปัจจุบัน ${temperatureCurrent.displayNumber()}°C"
        findViewById<ProgressBar>(R.id.temperatureProgress).progress = percent(temperatureCurrent, 100.0)
        findViewById<TextView>(R.id.currentHumidityText).text = "ปัจจุบัน ${humidityCurrent.displayNumber()}%"
        findViewById<ProgressBar>(R.id.humidityProgress).progress = percent(humidityCurrent, 100.0)
        findViewById<TextView>(R.id.uvSensorText).text = "${uv_raw.displayNumber()}" //mW/cm²

        findViewById<TextView>(R.id.doorSWStatusText).text = if (door_open) "เปิดอยู่" else "ปิดอยู่"

        findViewById<TextView>(R.id.lightStatus1).text = if (lamp1) "เปิดอยู่" else "ปิดอยู่"
        findViewById<TextView>(R.id.lightStatus2).text = if (lamp2) "เปิดอยู่" else "ปิดอยู่"
        findViewById<TextView>(R.id.lightStatus3).text = if (lamp3) "เปิดอยู่" else "ปิดอยู่"
        findViewById<TextView>(R.id.lightStatus4).text = if (lamp4) "เปิดอยู่" else "ปิดอยู่"

        findViewById<TextView>(R.id.motorStatusText).text = if (motor_status) "เปิดอยู่" else "ปิดอยู่"
    }

    private fun DataSnapshot.text(path: String, fallback: String): String {
        return child(path).getValue(String::class.java) ?: fallback
    }

    private fun DataSnapshot.boolean(path: String, fallback: Boolean): Boolean {
        return child(path).getValue(Boolean::class.java) ?: fallback
    }

    private fun DataSnapshot.double(path: String, fallback: Double): Double {
        return child(path).getValue(Double::class.java)
            ?: child(path).getValue(Long::class.java)?.toDouble()
            ?: fallback
    }

    private fun Double.displayNumber(): String {
        return if (this % 1.0 == 0.0) {
            toInt().toString()
        } else {
            String.format("%.1f", this)
        }
    }

    private fun percent(value: Double, max: Double): Int {
        if (max <= 0.0) return 0
        return ((value / max) * 100).toInt().coerceIn(0, 100)
    }

}