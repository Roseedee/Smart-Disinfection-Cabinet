package com.app.sdc.view

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.view.inputmethod.InputMethodManager
import android.widget.EditText
import android.widget.FrameLayout
import android.widget.TextView
import android.widget.Toast
import com.app.sdc.R
import com.app.sdc.UserSession
import com.google.firebase.FirebaseApp
import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.DatabaseReference
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ValueEventListener
import android.content.Intent
import com.app.sdc.StartupActivity

class SettingsView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    private lateinit var fullNameInput: EditText
    private lateinit var deviceSNInput: EditText
    private lateinit var btnSaveName: TextView
    private lateinit var btnLogOut: TextView

    private var userRef: DatabaseReference? = null
    private var userListener: ValueEventListener? = null

    private var originalFullName = ""

    private var userId = ""
    private var deviceSN = ""

    init {
        LayoutInflater.from(context).inflate(
            R.layout.view_settings,
            this,
            true
        )

        fullNameInput = findViewById(R.id.fullNameInput)
        deviceSNInput = findViewById(R.id.deviceSNInput)
        btnSaveName = findViewById(R.id.btnSaveName)
        btnLogOut = findViewById(R.id.btnLogOut)

        btnSaveName.visibility = GONE
        deviceSNInput.isEnabled = false

        userId = UserSession.getUserId(context) ?: ""
        deviceSN = UserSession.getDeviceSN(context) ?: ""

        deviceSNInput.setText(deviceSN)

        observeUser()
        setupListeners()
    }

    private fun setupListeners() {
        fullNameInput.setOnFocusChangeListener { _, hasFocus ->
            if (!hasFocus) {
                checkNameChanged()
            }
        }

        fullNameInput.addTextChangedListener(
            object : android.text.TextWatcher {
                override fun beforeTextChanged(
                    s: CharSequence?,
                    start: Int,
                    count: Int,
                    after: Int
                ) {
                }

                override fun onTextChanged(
                    s: CharSequence?,
                    start: Int,
                    before: Int,
                    count: Int
                ) {
                    checkNameChanged()
                }

                override fun afterTextChanged(
                    s: android.text.Editable?
                ) {
                }
            }
        )

        btnSaveName.setOnClickListener {
            saveFullName()
        }

        btnLogOut.setOnClickListener {
            logout()
        }
    }

    private fun observeUser() {
        if (FirebaseApp.getApps(context).isEmpty()) {
            return
        }

        if (userId.isEmpty()) {
            return
        }

        userRef = FirebaseDatabase.getInstance()
            .getReference("users")
            .child(userId)

        userListener = object : ValueEventListener {

            override fun onDataChange(snapshot: DataSnapshot) {
                val fullname =
                    snapshot.child("fullName")
                        .getValue(String::class.java)
                        ?: ""

                originalFullName = fullname

                fullNameInput.setText(fullname)

                btnSaveName.visibility = GONE
            }

            override fun onCancelled(error: DatabaseError) {
                Toast.makeText(
                    context,
                    "โหลดข้อมูลผู้ใช้ไม่สำเร็จ",
                    Toast.LENGTH_SHORT
                ).show()
            }
        }

        userRef?.addValueEventListener(userListener!!)
    }

    private fun checkNameChanged() {
        val currentName =
            fullNameInput.text
                .toString()
                .trim()

        btnSaveName.visibility =
            if (
                currentName != originalFullName &&
                currentName.isNotEmpty()
            ) {
                VISIBLE
            } else {
                GONE
            }
    }

    private fun saveFullName() {
        val fullname =
            fullNameInput.text
                .toString()
                .trim()

        if (fullname.isEmpty()) {
            Toast.makeText(
                context,
                "กรุณากรอกชื่อ-นามสกุล",
                Toast.LENGTH_SHORT
            ).show()

            return
        }

        if (userId.isEmpty()) {
            Toast.makeText(
                context,
                "ไม่พบ User ID",
                Toast.LENGTH_SHORT
            ).show()

            return
        }

        userRef = FirebaseDatabase.getInstance()
            .getReference("users")
            .child(userId)

        btnSaveName.isEnabled = false

        userRef?.child("fullName")
            ?.setValue(fullname)
            ?.addOnSuccessListener {

                UserSession.saveUser(
                    context,
                    userId,
                    fullname,
                    deviceSN
                )

                originalFullName = fullname
                fullNameInput.clearFocus()

                val imm =
                    context.getSystemService(Context.INPUT_METHOD_SERVICE)
                            as InputMethodManager

                imm.hideSoftInputFromWindow(
                    fullNameInput.windowToken,
                    0
                )

                btnSaveName.visibility = GONE
                btnSaveName.isEnabled = true

                Toast.makeText(
                    context,
                    "บันทึกชื่อเรียบร้อย",
                    Toast.LENGTH_SHORT
                ).show()
            }
            ?.addOnFailureListener {

                btnSaveName.isEnabled = true

                Toast.makeText(
                    context,
                    "บันทึกชื่อไม่สำเร็จ",
                    Toast.LENGTH_SHORT
                ).show()
            }
    }

    private fun logout() {
        clearUserSession()

        val intent =
            Intent(context, StartupActivity::class.java)

        intent.flags =
            Intent.FLAG_ACTIVITY_NEW_TASK or
                    Intent.FLAG_ACTIVITY_CLEAR_TASK

        context.startActivity(intent)
    }

    private fun clearUserSession() {
        UserSession.logout(context)
        Toast.makeText(context, "ออกจากระบบแล้ว", Toast.LENGTH_SHORT).show()
    }

    override fun onDetachedFromWindow() {

        if (
            userRef != null &&
            userListener != null
        ) {
            userRef?.removeEventListener(
                userListener!!
            )
        }

        userRef = null
        userListener = null

        super.onDetachedFromWindow()
    }
}