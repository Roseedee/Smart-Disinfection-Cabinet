package com.app.sdc.view

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.view.View
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
    private var taskRef: DatabaseReference? = null
    private var taskListener: ValueEventListener? = null

    private var devicesn: String
    private var taskRunningView: View

    var onAddTaskClick: (() -> Unit)? = null

    init {
        LayoutInflater.from(context).inflate(
            R.layout.view_dashboard,
            this,
            true
        )

        devicesn = UserSession.getDeviceSN(context) ?: ""

        taskRunningView = findViewById(R.id.taskRunning)
        taskRunningView.visibility = View.GONE

        observeDashboard()
        observeTask()

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

        dashboardRef?.addValueEventListener(dashboardListener!!)
    }

    private fun observeTask() {
        if (FirebaseApp.getApps(context).isEmpty()) {
            return
        }

        taskRef = FirebaseDatabase.getInstance()
            .getReference("devices")
            .child(devicesn)
            .child("task")

        taskListener = object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                if (snapshot.exists()) {
                    showTaskRunning(snapshot)
                } else {
                    hideTaskRunning()
                }
            }

            override fun onCancelled(error: DatabaseError) {
                hideTaskRunning()
            }
        }

        taskRef?.addValueEventListener(taskListener!!)
    }

    private fun showTaskRunning(snapshot: DataSnapshot) {
        taskRunningView.visibility = View.VISIBLE

        val userId = snapshot.child("userid").getValue(String::class.java) ?: ""
        val status = snapshot.child("status").getValue(String::class.java) ?: "running"
        val taskProgress = snapshot.long("progress", 0L)
        val lamp1 = snapshot.boolean("lamps/L1", false)
        val lamp2 = snapshot.boolean("lamps/L2", false)
        val lamp3 = snapshot.boolean("lamps/L3", false)
        val lamp4 = snapshot.boolean("lamps/L4", false)

        val ownerNameText = taskRunningView.findViewById<TextView>(R.id.ownerNameText)

        val stopTaskButton = taskRunningView.findViewById<TextView>(R.id.stopTaskButton)

        val cancelTaskButton = taskRunningView.findViewById<TextView>(R.id.cancelTaskButton)


        when (status) {
            "running" -> {
                stopTaskButton.text = "หยุด"

                stopTaskButton.setOnClickListener {
                    taskRef?.child("command")?.setValue("stop")
                }
            }

            "paused" -> {
                stopTaskButton.text = "เริ่มต่อ"

                stopTaskButton.setOnClickListener {
                    taskRef?.child("command")?.setValue("start")
                }
            }
        }

        cancelTaskButton.setOnClickListener {
            taskRef?.child("command")?.setValue("cancel")
        }

        val taskText = taskRunningView.findViewById<TextView>(R.id.tvTask)
        val dateText = taskRunningView.findViewById<TextView>(R.id.tvDate)
        val timeText = taskRunningView.findViewById<TextView>(R.id.tvTime)
        val progress = taskRunningView.findViewById<ProgressBar>(R.id.taskProgress)

        val lamps = listOf(
            lamp1,
            lamp2,
            lamp3,
            lamp4
        ).count { it }

        val duration = snapshot.long("duration", 0L)
        val startedAt = snapshot.long("started_at", 0L)
        val finishedAt = snapshot.long("finished_at", 0L)

        loadOwnerName(userId, ownerNameText)
        taskText.text = "สั่งงาน ${formatDuration(duration)} ${lamps} หลอด"
        dateText.text = "เวลาเริ่มทำงาน ${formatTime(startedAt)}"
        timeText.text = "เวลาสิ้นสุดทำงาน ${formatTime(finishedAt)}"

        progress.max = 100
        progress.progress = taskProgress.toInt().coerceIn(0, 100)
//        progress.progress = calculateTaskProgress(
//            startedAt,
//            finishedAt
//        )
    }

    private fun loadOwnerName(
        userId: String,
        textView: TextView
    ) {
        if (userId.isEmpty()) {
            textView.text = "ไม่ทราบชื่อ"
            return
        }

        FirebaseDatabase.getInstance()
            .getReference("users")
            .child(userId)
            .child("fullName")
            .get()
            .addOnSuccessListener { snapshot ->
                textView.text =
                    snapshot.getValue(String::class.java) ?: "ไม่ทราบชื่อ"
            }
            .addOnFailureListener {
                textView.text = "ไม่ทราบชื่อ"
            }
    }

    private fun hideTaskRunning() {
        taskRunningView.visibility = View.GONE
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

        findViewById<TextView>(R.id.currentTemperatureText).text =
            "ปัจจุบัน ${temperatureCurrent.displayNumber()}°C"

        findViewById<ProgressBar>(R.id.temperatureProgress).progress =
            percent(temperatureCurrent, 100.0)

        findViewById<TextView>(R.id.currentHumidityText).text =
            "ปัจจุบัน ${humidityCurrent.displayNumber()}%"

        findViewById<ProgressBar>(R.id.humidityProgress).progress =
            percent(humidityCurrent, 100.0)

        findViewById<TextView>(R.id.uvSensorText).text =
            uv_raw.displayNumber()

        findViewById<TextView>(R.id.doorSWStatusText).text =
            if (door_open) "เปิดอยู่" else "ปิดอยู่"

        findViewById<TextView>(R.id.lightStatus1).text =
            if (lamp1) "เปิดอยู่" else "ปิดอยู่"

        findViewById<TextView>(R.id.lightStatus2).text =
            if (lamp2) "เปิดอยู่" else "ปิดอยู่"

        findViewById<TextView>(R.id.lightStatus3).text =
            if (lamp3) "เปิดอยู่" else "ปิดอยู่"

        findViewById<TextView>(R.id.lightStatus4).text =
            if (lamp4) "เปิดอยู่" else "ปิดอยู่"

        findViewById<TextView>(R.id.motorStatusText).text =
            if (motor_status) "เปิดอยู่" else "ปิดอยู่"
    }

    private fun formatDuration(seconds: Long): String {
        val hours = seconds / 3600
        val minutes = (seconds % 3600) / 60
        val remainingSeconds = seconds % 60

        return when {
            hours > 0L && minutes > 0L && remainingSeconds > 0L ->
                "${hours} ชั่วโมง ${minutes} นาที ${remainingSeconds} วินาที"

            hours > 0L && minutes > 0L ->
                "${hours} ชั่วโมง ${minutes} นาที"

            hours > 0L && remainingSeconds > 0L ->
                "${hours} ชั่วโมง ${remainingSeconds} วินาที"

            hours > 0L ->
                "${hours} ชั่วโมง"

            minutes > 0L && remainingSeconds > 0L ->
                "${minutes} นาที ${remainingSeconds} วินาที"

            minutes > 0L ->
                "${minutes} นาที"

            else ->
                "${remainingSeconds} วินาที"
        }
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

    private fun DataSnapshot.long(path: String, fallback: Long): Long {
        return child(path).getValue(Long::class.java) ?: fallback
    }

//    private fun calculateTaskProgress(
//        startedAt: Long,
//        finishedAt: Long
//    ): Int {
//        if (startedAt <= 0L || finishedAt <= startedAt) {
//            return 0
//        }
//
//        val now = System.currentTimeMillis() / 1000
//
//        return (((now - startedAt).toDouble() /
//                (finishedAt - startedAt)) * 100)
//            .toInt()
//            .coerceIn(0, 100)
//    }

    private fun formatTime(timestamp: Long): String {
        if (timestamp <= 0L) return "--:--:--"

        return java.text.SimpleDateFormat(
            "HH:mm:ss",
            java.util.Locale.getDefault()
        ).format(
            java.util.Date(timestamp * 1000)
        )
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

    override fun onDetachedFromWindow() {
        dashboardRef?.let { ref ->
            dashboardListener?.let { listener ->
                ref.removeEventListener(listener)
            }
        }

        taskRef?.let { ref ->
            taskListener?.let { listener ->
                ref.removeEventListener(listener)
            }
        }

        dashboardRef = null
        dashboardListener = null
        taskRef = null
        taskListener = null

        super.onDetachedFromWindow()
    }
}