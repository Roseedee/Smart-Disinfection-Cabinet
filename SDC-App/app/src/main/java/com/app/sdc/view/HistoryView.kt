package com.app.sdc.view

import android.content.Context
import android.util.Log
import android.widget.FrameLayout
import com.app.sdc.R
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.app.sdc.UserSession
import com.app.sdc.adapter.TaskHistoryAdapter
import com.app.sdc.model.TaskHistory


import com.google.firebase.database.DataSnapshot
import com.google.firebase.database.DatabaseError
import com.google.firebase.database.FirebaseDatabase

import com.google.firebase.database.ValueEventListener

import java.util.Locale

class HistoryView(
    context: Context
) : FrameLayout(context) {

    private lateinit var recyclerHistory: RecyclerView

    private val historyList =
        mutableListOf<TaskHistory>()

    private lateinit var adapter: TaskHistoryAdapter

    init {

        inflate(
            context,
            R.layout.view_history,
            this
        )

        recyclerHistory =
            findViewById(R.id.recyclerHistory)

        setupRecyclerView()

        loadHistory()
    }


    private fun setupRecyclerView() {

        adapter =
            TaskHistoryAdapter(historyList)

        recyclerHistory.layoutManager =
            LinearLayoutManager(context)

        recyclerHistory.adapter =
            adapter
    }


    private fun loadHistory() {

        val userId = UserSession.getUserId(context)

        Log.d(
            "FirebaseDebug",
            "History UserID = $userId"
        )

        val ref = FirebaseDatabase.getInstance()
            .getReference("users")
            .child(userId ?: "")
            .child("history")

        Log.d(
            "FirebaseDebug",
            "History path = users/$userId/history"
        )

        ref.addListenerForSingleValueEvent(
            object : ValueEventListener {

                override fun onDataChange(snapshot: DataSnapshot) {

                    historyList.clear()

                    for (taskSnapshot in snapshot.children) {

                        val task = parseTask(taskSnapshot)

                        if (task != null) {
                            historyList.add(task)
                        }
                    }

                    // เรียงตาม task_<timestamp> ใหม่ -> เก่า
                    historyList.sortByDescending {
                        it.taskId
                    }

                    adapter.notifyDataSetChanged()

                    Log.d(
                        "FirebaseDebug",
                        "Loaded ${historyList.size} history items"
                    )
                }

                override fun onCancelled(error: DatabaseError) {

                    Log.e(
                        "FirebaseDebug",
                        "History onCancelled"
                    )

                    Log.e(
                        "FirebaseDebug",
                        "Error = ${error.message}"
                    )
                }
            }
        )
    }


    private fun parseTask(
        snapshot: DataSnapshot
    ): TaskHistory? {

        val taskId =
            snapshot.key ?: return null


        val duration =
            snapshot.child("duration")
                .getValue(Int::class.java)
                ?: 0


        val status =
            snapshot.child("status")
                .getValue(String::class.java)
                ?: ""


        val startedAt =
            snapshot.child("started_at")
                .getValue(String::class.java)


        val finishedAt =
            snapshot.child("finished_at")
                .getValue(String::class.java)


        val progress =
            snapshot.child("progress")
                .getValue(Int::class.java)
                ?: 0


        val remaining =
            snapshot.child("remaining")
                .getValue(Int::class.java)
                ?: 0


        val userId =
            snapshot.child("userid")
                .getValue(String::class.java)
                ?: ""


        val lamp1 =
            snapshot.child("lamps")
                .child("L1")
                .getValue(Boolean::class.java)
                ?: false


        val lamp2 =
            snapshot.child("lamps")
                .child("L2")
                .getValue(Boolean::class.java)
                ?: false


        val lamp3 =
            snapshot.child("lamps")
                .child("L3")
                .getValue(Boolean::class.java)
                ?: false


        val lamp4 =
            snapshot.child("lamps")
                .child("L4")
                .getValue(Boolean::class.java)
                ?: false


        return TaskHistory(

            taskId = taskId,

            duration = duration,

            status = status,

            startedAt = startedAt,

            finishedAt = finishedAt,

            lamp1 = lamp1,
            lamp2 = lamp2,
            lamp3 = lamp3,
            lamp4 = lamp4,

            progress = progress,

            remaining = remaining,

            userId = userId
        )
    }
}