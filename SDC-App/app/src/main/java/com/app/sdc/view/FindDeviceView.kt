package com.app.sdc.view

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.widget.FrameLayout
import android.widget.TextView
import com.app.sdc.R
import com.app.sdc.StartupActivity
import com.app.sdc.StartupRoute

class FindDeviceView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    init {
        LayoutInflater.from(context).inflate(
            R.layout.view_find_device,
            this,
            true
        )

        findViewById<TextView>(R.id.textFindDeviceView).text = "Find Device KT"

        var next = findViewById<TextView>(R.id.textFindDeviceView);
        next.setOnClickListener {
            (context as? StartupActivity)?.next(StartupRoute.DEVICE_CONNECTION)
        }
    }
}