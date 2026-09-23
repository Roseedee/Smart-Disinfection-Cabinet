package com.app.sdc

import android.content.Context

object UserSession {

    private const val PREF_NAME = "sdc_session"

    private const val KEY_USER_ID = "user_id"
    private const val KEY_FULL_NAME = "full_name"
    private const val KEY_DEVICE_SN = "device_sn"

    fun saveUser(
        context: Context,
        userId: String,
        fullName: String,
        deviceSN: String
    ) {

        context
            .getSharedPreferences(
                PREF_NAME,
                Context.MODE_PRIVATE
            )
            .edit()
            .putString(KEY_USER_ID, userId)
            .putString(KEY_FULL_NAME, fullName)
            .putString(KEY_DEVICE_SN, deviceSN)
            .apply()
    }

    fun getUserId(context: Context): String? {
        return context
            .getSharedPreferences(
                PREF_NAME,
                Context.MODE_PRIVATE
            )
            .getString(KEY_USER_ID, null)
    }

    fun getFullName(context: Context): String? {
        return context
            .getSharedPreferences(
                PREF_NAME,
                Context.MODE_PRIVATE
            )
            .getString(KEY_FULL_NAME, null)
    }

    fun getDeviceSN(context: Context): String? {
        return context
            .getSharedPreferences(
                PREF_NAME,
                Context.MODE_PRIVATE
            )
            .getString(KEY_DEVICE_SN, null)
    }

    fun isLoggedIn(context: Context): Boolean {
        return !getUserId(context).isNullOrEmpty()
    }

    fun logout(context: Context) {

        context
            .getSharedPreferences(
                PREF_NAME,
                Context.MODE_PRIVATE
            )
            .edit()
            .clear()
            .apply()
    }
}