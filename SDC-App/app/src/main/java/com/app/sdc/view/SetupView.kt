package com.app.sdc.view

import android.content.Context
import android.util.AttributeSet
import android.util.Log
import android.view.LayoutInflater
import android.widget.EditText
import android.widget.FrameLayout
import android.widget.TextView
import android.widget.Toast
import com.app.sdc.R
import com.app.sdc.StartupActivity
import com.app.sdc.UserSession
import com.google.firebase.messaging.FirebaseMessaging
import com.google.firebase.database.FirebaseDatabase

class SetupView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    private var fullName: String = ""
    private var deviceSN: String = ""

    private val database = FirebaseDatabase.getInstance()

    init {
        LayoutInflater.from(context).inflate(R.layout.view_setup, this, true)

        findViewById<TextView>(R.id.registerButton).setOnClickListener { onRegisterClick() }
    }

    private fun onRegisterClick() {
        fullName = findViewById<EditText>(R.id.fullNameInput).text.toString().trim()
        deviceSN = findViewById<EditText>(R.id.deviceCodeInput).text.toString().trim()

        Log.d("Register", "Full Name = $fullName")

        Log.d("Register", "Device SN = $deviceSN")

        // ตรวจสอบข้อมูล
        if (fullName.isEmpty()) {
            Toast.makeText(context, "กรุณากรอกชื่อ", Toast.LENGTH_SHORT).show()
            return
        }

        if (deviceSN.isEmpty()) {
            Toast.makeText(context, "กรุณากรอกรหัสอุปกรณ์", Toast.LENGTH_SHORT).show()
            return
        }

        // ตรวจสอบ Device ก่อน
        checkDevice()
    }

    private fun checkDevice() {

        Log.d("Register", "Checking device: $deviceSN")

        database.getReference("devices").child(deviceSN).get().addOnSuccessListener { snapshot ->
                if (!snapshot.exists()) {
                    Log.d("Register", "Device NOT FOUND: $deviceSN")
                    Toast.makeText(context, "ไม่พบอุปกรณ์รหัส $deviceSN", Toast.LENGTH_SHORT).show()
                    return@addOnSuccessListener
                }

                Log.d("Register", "Device FOUND: $deviceSN")
                getFCMToken()
            }
            .addOnFailureListener { error ->
                Log.e("Register", "Failed to check device", error)
                Toast.makeText(context, "ไม่สามารถตรวจสอบอุปกรณ์ได้", Toast.LENGTH_SHORT).show()
            }
    }

    private fun getFCMToken() {

        Log.d("Register", "Getting FCM Token...")
        FirebaseMessaging.getInstance().token.addOnCompleteListener { task ->
                if (!task.isSuccessful) {
                    Log.e("Register", "Failed to get FCM Token", task.exception)
                    Toast.makeText(context, "ไม่สามารถรับ FCM Token ได้", Toast.LENGTH_SHORT).show()
                    return@addOnCompleteListener
                }

                val fcmToken = task.result

                Log.d("Register", "FCM Token received")

                Log.d("Register", "FCM Token = $fcmToken")

                // ได้ Token แล้ว
                registerUser(fcmToken)
            }
    }

    private fun registerUser(fcmToken: String) {

        Log.d("Register", "Generating User ID...")

        val userRef =database.getReference("users").push()

        val userId = userRef.key

        if (userId == null) {
            Log.e("Register", "Failed to generate User ID")
            Toast.makeText(context, "ไม่สามารถสร้าง User ID ได้", Toast.LENGTH_SHORT).show()
            return
        }

        Log.d("Register", "User ID = $userId")

        val userData = mapOf("fullName" to fullName, "deviceSN" to deviceSN, "fcmToken" to fcmToken)

        Log.d("Register", "Saving user data...")

        userRef.setValue(userData).addOnSuccessListener {
            Log.d("Register", "================================")
            Log.d("Register","REGISTER SUCCESS")
            Log.d("Register","User ID = $userId")
            Log.d("Register","Full Name = $fullName")
            Log.d("Register","Device SN = $deviceSN")
            Log.d("Register","================================")
            UserSession.saveUser(context = context, userId = userId, fullName = fullName, deviceSN = deviceSN)
            Toast.makeText(context,"ลงทะเบียนสำเร็จ",Toast.LENGTH_SHORT).show()
            (context as? StartupActivity)?.openMainActivity()

            }.addOnFailureListener { error ->
                Log.e("Register","REGISTER FAILED",error)
                Toast.makeText(context,"ลงทะเบียนไม่สำเร็จ",Toast.LENGTH_SHORT).show()
            }
    }
}