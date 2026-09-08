package com.app.sdc.view

import android.content.Context
import android.service.autofill.Validators.not
import android.util.AttributeSet
import android.view.LayoutInflater
import android.widget.Button
import android.widget.FrameLayout
import android.widget.RadioButton
import android.widget.TextView
import android.widget.Toast
import com.google.android.material.switchmaterial.SwitchMaterial
import com.app.sdc.R
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL

class AddTaskView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    private lateinit var hourRadio: RadioButton
    private lateinit var minuteRadio: RadioButton
    private lateinit var secondRadio: RadioButton

    private var hours = 0
    private var minutes = 0
    private var seconds = 0

    private lateinit var lamp1Button: SwitchMaterial
    private lateinit var lamp2Button: SwitchMaterial
    private lateinit var lamp3Button: SwitchMaterial
    private lateinit var lamp4Button: SwitchMaterial

    private lateinit var addTaskButton: Button

    private val databaseUrl = "databaseurl"
    private val deviceId = "esp_001"

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
            Toast.makeText(context, "กรุณากำหนดเวลา", Toast.LENGTH_SHORT ).show()
            return
        }
        if(!lamp1Button.isChecked && !lamp2Button.isChecked && !lamp3Button.isChecked && !lamp4Button.isChecked) {
            Toast.makeText(context, "กรุณาเปิดไฟ", Toast.LENGTH_SHORT ).show()
            return
        }
        val taskId = generateTaskId()

        val lamps = JSONObject()
        lamps.put("L1", lamp1Button.isChecked)
        lamps.put("L2", lamp2Button.isChecked)
        lamps.put("L3", lamp3Button.isChecked)
        lamps.put("L4", lamp4Button.isChecked)

        val task = JSONObject()
        task.put("duration", duration)
        task.put("status", "pending")
        task.put("lamps",lamps)

        val urlString = "$databaseUrl/devices/$deviceId/tasks/$taskId.json"

        Thread {
            var connection: HttpURLConnection? = null
            try {
                val url = URL(urlString)
                connection = url.openConnection() as HttpURLConnection
                connection.requestMethod = "PUT"
                connection.setRequestProperty(
                    "Content-Type",
                    "application/json"
                )
                connection.doOutput = true
                connection.connectTimeout = 10000
                connection.readTimeout = 10000
                connection.outputStream.use { output -> output.write(task.toString().toByteArray(Charsets.UTF_8))}

                val responseCode = connection.responseCode

                post {
                    if (responseCode in 200..299) {
                        Toast.makeText(context, "เพิ่มงานสำเร็จ\n$taskId",Toast.LENGTH_SHORT).show()
                    } else {
                        Toast.makeText(context, "เพิ่มงานไม่สำเร็จ\nHTTP $responseCode",Toast.LENGTH_LONG).show()
                    }
                }
            } catch (e: Exception) {
                post {
                    Toast.makeText(
                        context,
                        "เกิดข้อผิดพลาด\n${e.message}",
                        Toast.LENGTH_LONG
                    ).show()
                }
            } finally {
                connection?.disconnect()
            }
        }.start()
    }
}