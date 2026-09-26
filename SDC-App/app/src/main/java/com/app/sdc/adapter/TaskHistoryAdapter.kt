package com.app.sdc.adapter

import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.cardview.widget.CardView
import androidx.recyclerview.widget.RecyclerView
import com.app.sdc.R
import com.app.sdc.model.TaskHistory
import java.util.Locale

class TaskHistoryAdapter(
    private val items: MutableList<TaskHistory>
) : RecyclerView.Adapter<TaskHistoryAdapter.TaskViewHolder>() {

    class TaskViewHolder(view: View) : RecyclerView.ViewHolder(view) {

        val card: CardView = view.findViewById(R.id.taskCard)
        val status: TextView = view.findViewById(R.id.tvStatus)
        val task: TextView = view.findViewById(R.id.tvTask)
        val date: TextView = view.findViewById(R.id.tvDate)
        val time: TextView = view.findViewById(R.id.tvTime)
    }

    override fun onCreateViewHolder(
        parent: ViewGroup,
        viewType: Int
    ): TaskViewHolder {

        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_task, parent, false)

        return TaskViewHolder(view)
    }

    override fun onBindViewHolder(
        holder: TaskViewHolder,
        position: Int
    ) {

        val item = items[position]

        // -------------------------
        // สถานะ
        // -------------------------

        holder.status.text = when (item.status.lowercase()) {

            "running" -> "กำลังดำเนิน"

            "completed",
            "success",
            "finished" -> "สำเร็จ"

            "failed" -> "ล้มเหลว"

            "cancelled",
            "canceled" -> "ยกเลิก"

            else -> item.status
        }

        holder.task.text = "สั่งงาน ${formatDuration(item.duration)} ${item.lampCount()} หลอด"

        holder.date.text = "วันที่ใช้งาน ${formatDate(item.startedAt)}"

        holder.time.text = "ช่วงเวลาทำงาน ${formatTime(item.startedAt)} ถึง ${formatTime(item.finishedAt)}"

        val backgroundColor = when (item.status.lowercase()) {
            "running" ->
                "#A8E957"
            "failed" ->
                "#FFB5B5"
            "cancelled",
            "canceled" ->
                "#F77F7F"
            "completed",
            "success",
            "finished" ->
                "#E5E5E5"
            else ->
                "#E5E5E5"
        }

        holder.card.setCardBackgroundColor(
            Color.parseColor(backgroundColor)
        )
    }

    override fun getItemCount(): Int {
        return items.size
    }

    private fun formatDuration(seconds: Int): String {
        val hours = seconds / 3600
        val minutes = (seconds % 3600) / 60
        val remainingSeconds = seconds % 60

        return when {
            hours > 0L && minutes > 0L && remainingSeconds > 0L ->
                "${hours} ชั่วโมง ${minutes} นาที ${remainingSeconds} วินาที"

            hours > 0L && minutes > 0L ->
                "${hours} ชั่วโมง ${minutes} นาที"

            hours > 0L && remainingSeconds > 0L ->
                "${hours} ชั่วโมง ${remainingSeconds} วินาที"

            hours > 0L ->
                "${hours} ชั่วโมง"

            minutes > 0L && remainingSeconds > 0L ->
                "${minutes} นาที ${remainingSeconds} วินาที"

            minutes > 0L ->
                "${minutes} นาที"

            else ->
                "${remainingSeconds} วินาที"
        }
    }


    private fun formatDate(value: String?): String {

        if (value.isNullOrEmpty())
            return "-"

        return try {
            val input = java.text.SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ssXXX", Locale.US )

            val output = java.text.SimpleDateFormat("dd/MM/yyyy", Locale.US)

            output.format(input.parse(value)!!)

        } catch (e: Exception) {
            "-"
        }
    }


    private fun formatTime(
        value: String?
    ): String {

        if (value.isNullOrEmpty())
            return "-"

        return try {

            val input =
                java.text.SimpleDateFormat(
                    "yyyy-MM-dd'T'HH:mm:ssXXX",
                    Locale.US
                )

            val output =
                java.text.SimpleDateFormat(
                    "HH:mm:ss",
                    Locale.US
                )

            output.format(input.parse(value)!!)

        } catch (e: Exception) {

            "-"
        }
    }
}