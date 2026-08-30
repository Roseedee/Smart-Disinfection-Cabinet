package com.app.sdc

import android.os.Bundle
import android.view.View
import android.widget.FrameLayout
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import com.app.sdc.view.DashboardView
import com.app.sdc.view.HistoryView
import com.app.sdc.view.SettingsView

class MainActivity : AppCompatActivity() {

    private lateinit var contentContainer: FrameLayout

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_main)

        applySystemBarsPadding()

        contentContainer = findViewById(R.id.contentContainer)

        setupMenu();

        showDashboard();
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
        contentContainer.removeAllViews()
        contentContainer.addView(
            DashboardView(this)
        )
        findViewById<TextView>(R.id.menuDashboard).isSelected = true;
        findViewById<TextView>(R.id.menuHistory).isSelected = false;
        findViewById<TextView>(R.id.menuSettings).isSelected = false;
    }

    private fun showHistory() {
        contentContainer.removeAllViews()
        contentContainer.addView(
            HistoryView(this)
        )
        findViewById<TextView>(R.id.menuDashboard).isSelected = false;
        findViewById<TextView>(R.id.menuHistory).isSelected = true;
        findViewById<TextView>(R.id.menuSettings).isSelected = false;
    }

    private fun showSettings() {
        contentContainer.removeAllViews()
        contentContainer.addView(
            SettingsView(this)
        )
        findViewById<TextView>(R.id.menuDashboard).isSelected = false;
        findViewById<TextView>(R.id.menuHistory).isSelected = false;
        findViewById<TextView>(R.id.menuSettings).isSelected = true;
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
}