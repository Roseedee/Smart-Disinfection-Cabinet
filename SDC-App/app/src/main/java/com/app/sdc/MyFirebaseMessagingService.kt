package com.app.sdc

import android.content.Intent
import android.util.Log
import com.google.firebase.messaging.FirebaseMessagingService
import com.google.firebase.messaging.RemoteMessage

class MyFirebaseMessagingService : FirebaseMessagingService() {

    override fun onNewToken(token: String) {
        super.onNewToken(token)

        Log.d(
            "FCM",
            "FCM Token = $token"
        )
    }

    override fun onMessageReceived(
        message: RemoteMessage
    ) {
        super.onMessageReceived(message)

        val title =
            message.notification?.title
                ?: "แจ้งเตือน"

        val body =
            message.notification?.body
                ?: ""

        Log.d(
            "FCM",
            "Message received"
        )

        Log.d(
            "FCM",
            "Title = $title"
        )

        Log.d(
            "FCM",
            "Body = $body"
        )

        val intent =
            Intent("com.app.sdc.FCM_NOTIFICATION")

        intent.setPackage(packageName)

        intent.putExtra(
            "title",
            title
        )

        intent.putExtra(
            "body",
            body
        )

        sendBroadcast(intent)

        Log.d(
            "FCM",
            "Broadcast sent"
        )
    }
}