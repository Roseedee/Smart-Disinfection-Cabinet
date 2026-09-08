package com.app.sdc.view

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.widget.FrameLayout
import android.widget.TextView
import com.app.sdc.R
import com.app.sdc.StartupActivity

class DeviceReadyView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    init {
        LayoutInflater.from(context).inflate(
            R.layout.view_device_ready,
            this,
            true
        )

        findViewById<TextView>(R.id.textDeviceReadyView).text = "Device Ready KT"
        var next = findViewById<TextView>(R.id.textDeviceReadyView);

        next.setOnClickListener {
            (context as? StartupActivity)?.openMainActivity()
        }
    }
}