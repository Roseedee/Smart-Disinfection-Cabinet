package com.app.sdc

import android.os.Bundle
import android.widget.FrameLayout
import androidx.appcompat.app.AppCompatActivity
import com.app.sdc.view.DeviceReadyView
import com.app.sdc.view.DeviceConnectionView
import com.app.sdc.view.FindDeviceView
import com.app.sdc.view.SetupView

class StartupActivity : AppCompatActivity() {

    private lateinit var startupContainer: FrameLayout

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContentView(R.layout.activity_startup)

        startupContainer = findViewById(R.id.startupContainer)

        route(StartupRoute.SETUP)
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