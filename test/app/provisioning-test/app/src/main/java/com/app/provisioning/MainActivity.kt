package com.app.provisioning

import android.content.Intent
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.os.Bundle
import android.provider.Settings
import android.view.View
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import kotlin.concurrent.thread

class MainActivity : AppCompatActivity() {

    private lateinit var tvStatus: TextView
    private lateinit var tvLog: TextView

    private lateinit var btnConnect: Button
    private lateinit var btnTestEsp32: Button
    private lateinit var btnScanWifi: Button

    private lateinit var wifiListContainer: LinearLayout

    private lateinit var connectivityManager: ConnectivityManager

    private var wifiCallback: ConnectivityManager.NetworkCallback? = null
    private var currentWifiNetwork: Network? = null

    companion object {

        private const val ESP32_SSID = "Sterilizer-001"
        private const val ESP32_IP = "192.168.4.1"

        private const val ESP32_SCAN_URL =
            "http://192.168.4.1/api/wifi/scan"

        private const val ESP32_CONNECT_URL =
            "http://192.168.4.1/api/wifi/connect"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        enableEdgeToEdge()

        setContentView(R.layout.activity_main)

        setupWindowInsets()
        setupViews()
        setupNetworkMonitor()
    }

    // =========================================================
    // WINDOW
    // =========================================================

    private fun setupWindowInsets() {

        val main = findViewById<View>(R.id.main)

        ViewCompat.setOnApplyWindowInsetsListener(main) { v, insets ->

            val systemBars =
                insets.getInsets(
                    WindowInsetsCompat.Type.systemBars()
                )

            v.setPadding(
                24 + systemBars.left,
                24 + systemBars.top,
                24 + systemBars.right,
                24 + systemBars.bottom
            )

            insets
        }
    }

    // =========================================================
    // VIEWS
    // =========================================================

    private fun setupViews() {

        tvStatus = findViewById(R.id.tvStatus)
        tvLog = findViewById(R.id.tvLog)

        btnConnect = findViewById(R.id.btnConnect)
        btnTestEsp32 = findViewById(R.id.btnTestEsp32)
        btnScanWifi = findViewById(R.id.btnScanWifi)

        wifiListContainer =
            findViewById(R.id.wifiListContainer)

        btnTestEsp32.isEnabled = false
        btnScanWifi.isEnabled = false

        btnConnect.setOnClickListener {
            openWifiSettings()
        }

        btnTestEsp32.setOnClickListener {
            testEsp32()
        }

        btnScanWifi.setOnClickListener {
            scanWifi()
        }
    }

    // =========================================================
    // OPEN WIFI SETTINGS
    // =========================================================

    private fun openWifiSettings() {

        tvStatus.text =
            "Status: Waiting for Wi-Fi"

        tvLog.text = """
            Log:
            Please connect to:

            $ESP32_SSID

            Password:
            12345678
        """.trimIndent()

        btnTestEsp32.isEnabled = false
        btnScanWifi.isEnabled = false

        startActivity(
            Intent(Settings.ACTION_WIFI_SETTINGS)
        )
    }

    // =========================================================
    // NETWORK MONITOR
    // =========================================================

    private fun setupNetworkMonitor() {

        connectivityManager =
            getSystemService(
                ConnectivityManager::class.java
            )

        val request =
            NetworkRequest.Builder()
                .addTransportType(
                    NetworkCapabilities.TRANSPORT_WIFI
                )
                .build()

        wifiCallback =
            object : ConnectivityManager.NetworkCallback() {

                override fun onAvailable(network: Network) {

                    currentWifiNetwork = network

                    runOnUiThread {

                        tvStatus.text =
                            "Status: Wi-Fi Connected"

                        tvLog.text = """
                            Log:
                            Wi-Fi network detected.

                            Device:
                            $ESP32_SSID

                            ESP32 IP:
                            $ESP32_IP

                            Ready.
                        """.trimIndent()

                        btnTestEsp32.isEnabled = true
                        btnScanWifi.isEnabled = true
                    }
                }

                override fun onLost(network: Network) {

                    if (network == currentWifiNetwork) {

                        currentWifiNetwork = null

                        runOnUiThread {

                            tvStatus.text =
                                "Status: Wi-Fi Disconnected"

                            tvLog.text =
                                "Log:\nWi-Fi connection lost."

                            btnTestEsp32.isEnabled = false
                            btnScanWifi.isEnabled = false
                        }
                    }
                }
            }

        connectivityManager.registerNetworkCallback(
            request,
            wifiCallback!!
        )
    }

    // =========================================================
    // TEST ESP32
    // =========================================================

    private fun testEsp32() {

        val network = currentWifiNetwork

        if (network == null) {

            tvStatus.text =
                "Status: No Wi-Fi Network"

            return
        }

        tvStatus.text =
            "Status: Testing ESP32..."

        btnTestEsp32.isEnabled = false

        thread {

            var connection: HttpURLConnection? = null

            try {

                val url =
                    URL("http://$ESP32_IP/")

                connection =
                    network.openConnection(url)
                            as HttpURLConnection

                connection.requestMethod = "GET"
                connection.connectTimeout = 5000
                connection.readTimeout = 5000
                connection.useCaches = false

                val responseCode =
                    connection.responseCode

                val responseText =
                    connection.inputStream
                        .bufferedReader()
                        .use { it.readText() }

                runOnUiThread {

                    if (responseCode == 200) {

                        tvStatus.text =
                            "Status: ESP32 Connected ✅"

                        tvLog.text = """
                            Log:
                            HTTP Request Success

                            Response Code:
                            $responseCode

                            Response:
                            $responseText
                        """.trimIndent()

                    } else {

                        tvStatus.text =
                            "Status: HTTP Error ❌"

                        tvLog.text =
                            "Response Code: $responseCode"
                    }

                    btnTestEsp32.isEnabled = true
                }

            } catch (e: Exception) {

                runOnUiThread {

                    tvStatus.text =
                        "Status: HTTP Failed ❌"

                    tvLog.text = """
                        HTTP Request Failed

                        ${e.javaClass.simpleName}

                        ${e.message}
                    """.trimIndent()

                    btnTestEsp32.isEnabled = true
                }

            } finally {

                connection?.disconnect()
            }
        }
    }

    // =========================================================
    // SCAN WIFI
    // =========================================================

    private fun scanWifi() {

        val network = currentWifiNetwork

        if (network == null) {

            tvStatus.text =
                "Status: ESP32 not connected"

            return
        }

        tvStatus.text =
            "Status: Scanning Wi-Fi..."

        tvLog.text = """
            Log:
            Requesting Wi-Fi list
            from ESP32...
        """.trimIndent()

        btnScanWifi.isEnabled = false

        wifiListContainer.removeAllViews()

        thread {

            var connection: HttpURLConnection? = null

            try {

                val url =
                    URL(ESP32_SCAN_URL)

                connection =
                    network.openConnection(url)
                            as HttpURLConnection

                connection.requestMethod = "GET"
                connection.connectTimeout = 10000
                connection.readTimeout = 10000
                connection.useCaches = false

                val responseCode =
                    connection.responseCode

                val responseText =
                    connection.inputStream
                        .bufferedReader()
                        .use { it.readText() }

                if (responseCode != 200) {
                    throw Exception(
                        "HTTP $responseCode"
                    )
                }

                val json =
                    JSONObject(responseText)

                val networks =
                    json.getJSONArray("networks")

                runOnUiThread {

                    tvStatus.text =
                        "Status: Scan Complete ✅"

                    tvLog.text = """
                        Log:
                        Wi-Fi scan successful.

                        Networks found:
                        ${networks.length()}
                    """.trimIndent()

                    for (i in 0 until networks.length()) {

                        val item =
                            networks.getJSONObject(i)

                        val ssid =
                            item.getString("ssid")

                        val rssi =
                            item.getInt("rssi")

                        addWifiItem(
                            ssid,
                            rssi
                        )
                    }

                    btnScanWifi.isEnabled = true
                }

            } catch (e: Exception) {

                runOnUiThread {

                    tvStatus.text =
                        "Status: Scan Failed ❌"

                    tvLog.text = """
                        Wi-Fi Scan Failed

                        ${e.javaClass.simpleName}

                        ${e.message}
                    """.trimIndent()

                    btnScanWifi.isEnabled = true
                }

            } finally {

                connection?.disconnect()
            }
        }
    }

    // =========================================================
    // ADD WIFI ITEM
    // =========================================================

    private fun addWifiItem(
        ssid: String,
        rssi: Int
    ) {

        val item =
            TextView(this)

        item.text = """
            $ssid
            Signal: $rssi dBm
        """.trimIndent()

        item.textSize = 17f

        item.setPadding(
            20,
            20,
            20,
            20
        )

        item.setBackgroundResource(
            android.R.drawable.list_selector_background
        )

        // =============================================
        // กด SSID
        // =============================================

        item.setOnClickListener {

            showPasswordDialog(
                ssid
            )
        }

        val params =
            LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
            )

        params.setMargins(
            0,
            0,
            0,
            8
        )

        wifiListContainer.addView(
            item,
            params
        )
    }

    // =========================================================
    // PASSWORD DIALOG
    // =========================================================

    private fun showPasswordDialog(
        ssid: String
    ) {

        val input =
            EditText(this)

        input.hint =
            "Wi-Fi Password"

        input.inputType =
            android.text.InputType.TYPE_CLASS_TEXT or
                    android.text.InputType.TYPE_TEXT_VARIATION_PASSWORD

        input.setSingleLine(true)

        val container =
            LinearLayout(this)

        container.orientation =
            LinearLayout.VERTICAL

        container.setPadding(
            40,
            0,
            40,
            0
        )

        container.addView(input)


        val dialog =
            AlertDialog.Builder(this)
                .setTitle("Connect Wi-Fi")
                .setMessage(ssid)
                .setView(container)
                .setNegativeButton("CANCEL", null)
                .setPositiveButton("CONNECT", null)
                .create()


        dialog.setOnShowListener {

            val button =
                dialog.getButton(
                    AlertDialog.BUTTON_POSITIVE
                )

            button.setOnClickListener {

                val password =
                    input.text
                        .toString()

                if (password.isEmpty()) {

                    input.error =
                        "Please enter password"

                    return@setOnClickListener
                }


                dialog.dismiss()


                connectWifi(
                    ssid,
                    password
                )
            }
        }


        dialog.show()
    }

    // =========================================================
    // CONNECT WIFI
    // =========================================================

    private fun connectWifi(
        ssid: String,
        password: String
    ) {

        val network =
            currentWifiNetwork

        if (network == null) {

            tvStatus.text =
                "Status: ESP32 disconnected"

            return
        }


        tvStatus.text =
            "Status: Connecting..."


        tvLog.text = """
            Log:
            Sending Wi-Fi credentials...

            SSID:
            $ssid
        """.trimIndent()


        btnScanWifi.isEnabled =
            false


        thread {

            var connection:
                    HttpURLConnection? = null


            try {

                val url =
                    URL(
                        ESP32_CONNECT_URL
                    )


                connection =
                    network.openConnection(url)
                            as HttpURLConnection


                connection.requestMethod =
                    "POST"

                connection.connectTimeout =
                    10000

                connection.readTimeout =
                    10000

                connection.doOutput =
                    true

                connection.setRequestProperty(
                    "Content-Type",
                    "application/json"
                )

                connection.setRequestProperty(
                    "Accept",
                    "application/json"
                )


                // =========================================
                // JSON
                // =========================================

                val json =
                    JSONObject().apply {

                        put(
                            "ssid",
                            ssid
                        )

                        put(
                            "password",
                            password
                        )
                    }


                val body =
                    json.toString()


                connection.outputStream
                    .use { output ->

                        output.write(
                            body.toByteArray(
                                Charsets.UTF_8
                            )
                        )
                    }


                val responseCode =
                    connection.responseCode


                val responseText =
                    if (
                        responseCode in 200..299
                    ) {

                        connection.inputStream
                            .bufferedReader()
                            .use {
                                it.readText()
                            }

                    } else {

                        connection.errorStream
                            ?.bufferedReader()
                            ?.use {
                                it.readText()
                            }
                            ?: ""
                    }


                runOnUiThread {

                    if (
                        responseCode in 200..299
                    ) {

                        tvStatus.text =
                            "Status: ESP32 Connecting..."

                        tvLog.text = """
                            Log:
                            Credentials sent successfully.

                            SSID:
                            $ssid

                            HTTP:
                            $responseCode

                            ESP32 Response:
                            $responseText
                        """.trimIndent()

                    } else {

                        tvStatus.text =
                            "Status: Connect Failed ❌"

                        tvLog.text = """
                            HTTP Error:
                            $responseCode

                            Response:
                            $responseText
                        """.trimIndent()
                    }


                    btnScanWifi.isEnabled =
                        true
                }

            } catch (e: Exception) {

                runOnUiThread {

                    tvStatus.text =
                        "Status: Connect Failed ❌"

                    tvLog.text = """
                        Connection Request Failed

                        Error:
                        ${e.javaClass.simpleName}

                        Message:
                        ${e.message}
                    """.trimIndent()

                    btnScanWifi.isEnabled =
                        true
                }

            } finally {

                connection?.disconnect()
            }
        }
    }

    // =========================================================
    // CLEANUP
    // =========================================================

    override fun onDestroy() {

        wifiCallback?.let {

            try {

                connectivityManager
                    .unregisterNetworkCallback(it)

            } catch (_: Exception) {
            }
        }

        wifiCallback = null
        currentWifiNetwork = null

        super.onDestroy()
    }
}