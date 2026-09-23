package com.app.sdc.view

import android.content.Context
import android.util.AttributeSet
import android.util.Log
import android.view.LayoutInflater
import android.widget.Button
import android.widget.CheckBox
import android.widget.FrameLayout
import android.widget.RadioButton
import android.widget.TextView
import android.widget.Toast
import com.google.android.material.switchmaterial.SwitchMaterial
import com.app.sdc.R
import com.app.sdc.UserSession
import com.google.firebase.database.FirebaseDatabase

class AddTaskView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    private var hourRadio: RadioButton
    private var minuteRadio: RadioButton
    private var secondRadio: RadioButton

    private var hours = 0
    private var minutes = 0
    private var seconds = 0

    private var lamp1Button: SwitchMaterial
    private var lamp2Button: SwitchMaterial
    private var lamp3Button: SwitchMaterial
    private var lamp4Button: SwitchMaterial

    init {
        LayoutInflater.from(context).inflate(R.layout.view_add_task, this, true)

        hourRadio = findViewById(R.id.hourRadio)
        minuteRadio = findViewById(R.id.minuteRadio)
        secondRadio = findViewById(R.id.secondRadio)

        secondRadio.isChecked = true

        val plusButton = findViewById<TextView>(R.id.plusButton)
        val minusButton = findViewById<TextView>(R.id.minusButton)

        plusButton.setOnClickListener {
            increaseSelectedTime()
        }

        minusButton.setOnClickListener {
            decreaseSelectedTime()
        }

        updateTime()

        lamp1Button = findViewById(R.id.lamp1Button)
        lamp2Button = findViewById(R.id.lamp2Button)
        lamp3Button = findViewById(R.id.lamp3Button)
        lamp4Button = findViewById(R.id.lamp4Button)

        val addTaskButton = findViewById<Button>(R.id.addTaskButton)
        addTaskButton.setOnClickListener {
            uploadTaskToFirebase()
        }
    }

    var onFinished: (() -> Unit)? = null

    private fun increaseSelectedTime() {
        when {
            hourRadio.isChecked -> {
                if (hours < 23) {
                    hours++
                }
            }
            minuteRadio.isChecked -> {
                if (minutes < 59) {
                    minutes++
                }
            }
            secondRadio.isChecked -> {
                if (seconds < 59) {
                    seconds++
                }
            }
        }
        updateTime()
    }

    private fun decreaseSelectedTime() {
        when {
            hourRadio.isChecked -> {
                if (hours > 0) {
                    hours--
                }
            }
            minuteRadio.isChecked -> {
                if (minutes > 0) {
                    minutes--
                }
            }
            secondRadio.isChecked -> {
                if (seconds > 0) {
                    seconds--
                }
            }
        }
        updateTime()
    }

    private fun updateTime() {
        hourRadio.text = String.format("%02d", hours)
        minuteRadio.text = String.format("%02d", minutes)
        secondRadio.text = String.format("%02d", seconds)
    }

    private fun getDuration(): Int {
        return (hours * 3600) + (minutes * 60) + seconds
    }

    private fun generateTaskId(): String {
        return "task_" + System.currentTimeMillis()
    }

    private fun uploadTaskToFirebase() {
        val duration = getDuration()

        if (duration <= 0) {
            Toast.makeText(context, "กรุณากำหนดเวลา", Toast.LENGTH_SHORT).show()
            return
        }

        if (!lamp1Button.isChecked &&
            !lamp2Button.isChecked &&
            !lamp3Button.isChecked &&
            !lamp4Button.isChecked
        ) {
            Toast.makeText(context, "กรุณาเปิดไฟ", Toast.LENGTH_SHORT).show()
            return
        }

        val taskId = generateTaskId()
        val userId = UserSession.getUserId(context)
        val deviceSN = UserSession.getDeviceSN(context)

        if (userId.isNullOrEmpty()) {
            Toast.makeText(context, "ไม่พบข้อมูลผู้ใช้", Toast.LENGTH_SHORT).show()
            return
        }

        if (deviceSN.isNullOrEmpty()) {
            Toast.makeText(context, "ไม่พบข้อมูลอุปกรณ์", Toast.LENGTH_SHORT).show()
            return
        }

        val lamps = mapOf(
            "L1" to lamp1Button.isChecked,
            "L2" to lamp2Button.isChecked,
            "L3" to lamp3Button.isChecked,
            "L4" to lamp4Button.isChecked
        )

        val notifyFinish = findViewById<CheckBox>(R.id.notifyCheckBox).isChecked

        val task = mapOf(
            "task_id" to taskId,
            "userid" to userId,
            "duration" to duration,
            "remaining" to duration,
            "status" to "pending",
            "command" to "start",
            "notify_finish" to notifyFinish,
            "lamps" to lamps
        )

        Log.d("FirebaseTask", "Device = $deviceSN")
        Log.d("FirebaseTask", "Task = $task")

        val taskRef = FirebaseDatabase.getInstance()
            .getReference("devices")
            .child(deviceSN)
            .child("task")

        taskRef.setValue(task)
            .addOnSuccessListener {
                Log.d("FirebaseTask", "Upload success")
                Toast.makeText(context, "เพิ่มงานสำเร็จ\n$taskId", Toast.LENGTH_SHORT).show()
                onFinished?.invoke()
            }
            .addOnFailureListener { e ->
                Log.e("FirebaseTask", "Upload failed", e)
                Toast.makeText(context, "เพิ่มงานไม่สำเร็จ\n${e.message}", Toast.LENGTH_LONG).show()
                onFinished?.invoke()
            }


    }
}