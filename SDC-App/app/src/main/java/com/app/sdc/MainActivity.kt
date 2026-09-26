package com.app.sdc

import android.annotation.SuppressLint
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.View
import android.widget.FrameLayout
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import com.app.sdc.view.AddTaskView
import com.app.sdc.view.DashboardView
import com.app.sdc.view.HistoryView
import com.app.sdc.view.SettingsView
import com.google.firebase.FirebaseApp
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import android.util.Log
import com.google.android.material.snackbar.Snackbar

class MainActivity : AppCompatActivity() {

    private lateinit var contentContainer: FrameLayout

    private var deviceRef: DatabaseReference? = null
    private var deviceListener: ValueEventListener? = null
    private lateinit var deviceSn: String
    private var lastSeen: Long = 0L
    private var deviceOnline: Boolean = false
    private var deviceBusy: Boolean = false

    private var notificationReceiver: BroadcastReceiver? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_main)

        notificationReceiver =
            object : BroadcastReceiver() {

                override fun onReceive(
                    context: Context?,
                    intent: Intent?
                ) {
                    Log.d(
                        "FCM_UI",
                        "Broadcast received"
                    )

                    val title =
                        intent?.getStringExtra("title")
                            ?: return

                    val body =
                        intent.getStringExtra("body")
                            ?: ""

                    Log.d(
                        "FCM_UI",
                        "Title = $title"
                    )

                    Log.d(
                        "FCM_UI",
                        "Body = $body"
                    )

                    showNotificationBanner(
                        title,
                        body
                    )
                }
            }

        registerReceiver(
            notificationReceiver,
            IntentFilter(
                "com.app.sdc.FCM_NOTIFICATION"
            ),
            Context.RECEIVER_NOT_EXPORTED
        )

        deviceSn = UserSession.getDeviceSN(this) ?: ""

        applySystemBarsPadding()

        contentContainer = findViewById(R.id.contentContainer)

        setupMenu()

        observeDeviceOnline()

        onlineHandler.post(onlineChecker)

        showDashboard()


    }

    private fun showNotificationBanner(
        title: String,
        body: String
    ) {

        val container = findViewById<FrameLayout>(R.id.notificationContainer)

        container.removeAllViews()

        val banner = layoutInflater.inflate(R.layout.notification_banner, container, false)

        val titleText = banner.findViewById<TextView>(R.id.notificationTitle)

        val bodyText = banner.findViewById<TextView>(R.id.notificationBody)

        titleText.text = title
        bodyText.text = body

        container.addView(banner)

        banner.translationY = -banner.height.toFloat()

        banner.post {
            banner.translationY = -banner.height.toFloat()

            banner.animate().translationY(0f).setDuration(300).start()

            banner.postDelayed({
                banner.animate().translationY(-banner.height.toFloat()).setDuration(300).withEndAction {
                        container.removeView(banner)
                    }.start()
            }, 4000)
        }
    }


    private val onlineHandler = Handler(Looper.getMainLooper())

    private val onlineChecker =
        object : Runnable {

            override fun run() {
                updateOnlineStatus()
                onlineHandler.postDelayed(this, 1000)
            }
        }

    private fun observeDeviceOnline() {
        Log.d("FirebaseDebug", "================================")
        Log.d("FirebaseDebug", "Starting Firebase connection...")
        Log.d("FirebaseDebug", "Device SN = $deviceSn")

        if (FirebaseApp.getApps(this).isEmpty()) {
            Log.e(
                "FirebaseDebug",
                "FirebaseApp is EMPTY - Firebase is NOT initialized"
            )
            return
        }

        Log.d(
            "FirebaseDebug",
            "FirebaseApp initialized successfully"
        )

        deviceRef = FirebaseDatabase.getInstance().getReference("devices").child(deviceSn)
        Log.d(
            "FirebaseDebug",
            "Firebase path = devices/$deviceSn"
        )


        deviceListener = object : ValueEventListener {
                override fun onDataChange(
                    snapshot: DataSnapshot
                ) {

                    Log.d(
                        "FirebaseDebug",
                        "Firebase onDataChange()"
                    )

                    lastSeen = snapshot.child("lastseen").getValue(Long::class.java) ?: 0L
                    deviceBusy = snapshot.child("hw_status").child("busy").getValue(Boolean::class.java) ?: false

                    updateOnlineStatus()
                }

                override fun onCancelled(
                    error: DatabaseError
                ) {

                    Log.e(
                        "FirebaseDebug",
                        "Firebase onCancelled()"
                    )

                    Log.e(
                        "FirebaseDebug",
                        "Error message = ${error.message}"
                    )

                    updateOnlineUI(
                        deviceOnline = false,
                        usageStatus = "เกิดข้อผิดพลาด"
                    )
                }
            }

        deviceRef?.addValueEventListener(
            deviceListener!!
        )
    }

    private fun updateOnlineStatus() {

        val now = System.currentTimeMillis() / 1000

        val difference = if (lastSeen > 0L) { now - lastSeen } else { -1L }

        deviceOnline = lastSeen > 0L && difference <= 20L

        val usageStatus = when {
            !deviceOnline ->
                "อุปกรณ์ออฟไลน์"
            deviceBusy ->
                "กำลังทำงาน"
            else ->
                "ยังไม่มีการใช้งาน"
        }
        updateOnlineUI(
            deviceOnline = deviceOnline,
            usageStatus = usageStatus
        )
    }

    @SuppressLint("UseCompatLoadingForDrawables")
    private fun updateOnlineUI(deviceOnline: Boolean, usageStatus: String) {
        val deviceSNText = contentContainer.findViewById<TextView>(R.id.deviceSerialText)
        val onlineText = contentContainer.findViewById<TextView>(R.id.onlineStatusText)
        val usageText = contentContainer.findViewById<TextView>(R.id.usageStatusText)
        val onlineDot = contentContainer.findViewById<View>(R.id.onlineDot)

        if (onlineText == null) {
            return
        }

        deviceSNText.text = deviceSn

        onlineText.text = if (deviceOnline) { "Online" } else { "Offline" }

        usageText?.text = usageStatus

        if (deviceOnline) {
            onlineText.setTextColor(
                getColor(
                    R.color.connection_online
                )
            )
            onlineDot?.background = getDrawable(R.drawable.bg_online_dot)
        } else {
            onlineText.setTextColor(
                getColor(
                    R.color.connection_offline
                )
            )
            onlineDot?.background = getDrawable(R.drawable.bg_offline_dot)
        }
    }

    private fun setupMenu() {
        findViewById<View>(R.id.menuDashboard).setOnClickListener {
            showDashboard()
        }
        findViewById<View>(R.id.menuHistory).setOnClickListener {
            showHistory()
        }
        findViewById<View>(R.id.menuSettings).setOnClickListener {
            showSettings()
        }
    }

    private fun showDashboard() {
        val dashboardView = DashboardView(this)

        dashboardView.onAddTaskClick = onAddTaskClick@{

            if (deviceBusy) {
                Snackbar.make(findViewById(R.id.main), "อุปกรณ์กำลังทำงานอยู่", Snackbar.LENGTH_LONG).show()
                return@onAddTaskClick
            }

            if (!deviceOnline) {
                Snackbar.make(findViewById(R.id.main), "อุปกรณ์ Offline อยู่", Snackbar.LENGTH_LONG).show()
                return@onAddTaskClick
            }

            val addTaskView = AddTaskView(this)

            addTaskView.onFinished = {
                showDashboard()
            }

            contentContainer.removeAllViews()
            contentContainer.addView(addTaskView)
        }
        contentContainer.removeAllViews()
        contentContainer.addView(
            dashboardView
        )
        findViewById<TextView>(R.id.menuDashboard).isSelected = true
        findViewById<TextView>(R.id.menuHistory).isSelected = false
        findViewById<TextView>(R.id.menuSettings).isSelected = false
    }

    private fun showHistory() {
        contentContainer.removeAllViews()
        contentContainer.addView(
            HistoryView(this)
        )
        findViewById<TextView>(R.id.menuDashboard).isSelected = false
        findViewById<TextView>(R.id.menuHistory).isSelected = true
        findViewById<TextView>(R.id.menuSettings).isSelected = false
    }

    private fun showSettings() {
        contentContainer.removeAllViews()
        contentContainer.addView(
            SettingsView(this)
        )
        findViewById<TextView>(R.id.menuDashboard).isSelected = false
        findViewById<TextView>(R.id.menuHistory).isSelected = false
        findViewById<TextView>(R.id.menuSettings).isSelected = true
    }

    private fun applySystemBarsPadding() {

        ViewCompat.setOnApplyWindowInsetsListener(
            findViewById(R.id.main)
        ) { view, insets ->

            val systemBars =
                insets.getInsets(WindowInsetsCompat.Type.systemBars())

            view.setPadding(
                systemBars.left,
                systemBars.top,
                systemBars.right,
                systemBars.bottom
            )

            insets
        }
    }

    override fun onDestroy() {

        notificationReceiver?.let {
            unregisterReceiver(it)
        }

        notificationReceiver = null

        onlineHandler.removeCallbacks(
            onlineChecker
        )

        if (
            deviceRef != null &&
            deviceListener != null
        ) {
            deviceRef?.removeEventListener(
                deviceListener!!
            )
        }

        deviceRef = null
        deviceListener = null

        super.onDestroy()
    }
}