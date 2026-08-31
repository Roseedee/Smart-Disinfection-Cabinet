package com.app.sdc.view

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.widget.FrameLayout
import android.widget.TextView
import com.app.sdc.R
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

    init {
        LayoutInflater.from(context).inflate(
            R.layout.view_dashboard,
            this,
            true
        )

        observeDashboard()
    }

    private fun observeDashboard() {
        if (FirebaseApp.getApps(context).isEmpty()) {
            return
        }

        dashboardRef = FirebaseDatabase.getInstance()
            .getReference("devices")
            .child("WE16WE1V6W")
            .child("dashboard")

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
        val serial = snapshot.text("serialNumber", "none")
        val isOnline = snapshot.boolean("online", true)
        val temperatureCurrent = snapshot.double("temperature/current", 32.0)
        val temperatureMax = snapshot.double("temperature/max", 100.0)
        val humidityCurrent = snapshot.double("humidity/current", 62.0)
        val humidityMax = snapshot.double("humidity/max", 100.0)

        findViewById<TextView>(R.id.deviceNameText).text = snapshot.text("deviceName", "เครื่องอบฆ่าเชื้อ")
        findViewById<TextView>(R.id.deviceSerialText).text = serial

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