package com.app.sdc

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.app.sdc.view.DeviceReadyView
import com.app.sdc.view.DeviceConnectionView
import com.app.sdc.view.FindDeviceView
import com.app.sdc.view.SetupView
import com.google.firebase.messaging.FirebaseMessaging

class StartupActivity : AppCompatActivity() {

    private lateinit var startupContainer: FrameLayout

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (UserSession.isLoggedIn(this)) {
            openMainActivity()
            return
        }

        setContentView(R.layout.activity_startup)

        startupContainer = findViewById(R.id.startupContainer)

        route(StartupRoute.SETUP)

        getFCMToken()
        requestNotificationPermission()
    }

    private fun getFCMToken() {

        FirebaseMessaging.getInstance().token
            .addOnCompleteListener { task ->

                if (!task.isSuccessful) {

                    Log.e(
                        "FCM",
                        "Failed to get FCM token",
                        task.exception
                    )

                    return@addOnCompleteListener
                }

                val token = task.result

                Log.d(
                    "FCM",
                    "FCM TOKEN = $token"
                )
            }
    }

    private fun requestNotificationPermission() {

        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.TIRAMISU) {

            if (
                ContextCompat.checkSelfPermission(
                    this,
                    Manifest.permission.POST_NOTIFICATIONS
                ) != PackageManager.PERMISSION_GRANTED
            ) {

                ActivityCompat.requestPermissions(
                    this,
                    arrayOf(Manifest.permission.POST_NOTIFICATIONS),
                    1001
                )
            }
        }
    }

    // =====================================================
    // ROUTE
    // =====================================================

    private fun route(route: StartupRoute) {

        startupContainer.removeAllViews()

        when (route) {

            StartupRoute.SETUP -> {
                startupContainer.addView(
                    SetupView(this)
                )
            }

            StartupRoute.FIND_DEVICE -> {
                startupContainer.addView(
                    FindDeviceView(this)
                )
            }

            StartupRoute.DEVICE_CONNECTION -> {
                startupContainer.addView(
                    DeviceConnectionView(this)
                )
            }

            StartupRoute.DEVICE_READY -> {
                startupContainer.addView(
                    DeviceReadyView(this)
                )
            }
        }
    }

    // =====================================================
    // ROUTE NEXT
    // =====================================================

    fun next(route: StartupRoute) {
        route(route)
    }

    // =====================================================
    // OPEN MAIN
    // =====================================================

    fun openMainActivity() {

        startActivity(
            android.content.Intent(
                this,
                MainActivity::class.java
            )
        )

        finish()
    }
}