package com.app.sdc.model

data class TaskHistory(
    val taskId: String = "",
    val duration: Int = 0,

    val status: String = "",

    val startedAt: String? = null,
    val finishedAt: String? = null,

    val lamp1: Boolean = false,
    val lamp2: Boolean = false,
    val lamp3: Boolean = false,
    val lamp4: Boolean = false,

    val progress: Int = 0,
    val remaining: Int = 0,

    val userId: String = ""
) {
    fun lampCount(): Int {
        return listOf(
            lamp1,
            lamp2,
            lamp3,
            lamp4
        ).count { it }
    }
}

enum class TaskStatus {
    RUNNING,
    FAILED,
    CANCELLED,
    SUCCESS
}