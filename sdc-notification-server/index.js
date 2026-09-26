const { initializeApp, cert } = require("firebase-admin/app");
const { getDatabase } = require("firebase-admin/database");
const { getMessaging } = require("firebase-admin/messaging");

const serviceAccount = require("./serviceAccountKey.json");

initializeApp({
    credential: cert(serviceAccount),
    databaseURL: "https://esp-lab-c89db-default-rtdb.asia-southeast1.firebasedatabase.app"
});

const db = getDatabase();
const messaging = getMessaging();

const deviceSn = "AWE416E1W61";

const taskRef =
    db.ref(`devices/${deviceSn}/task`);

let cleanupTimer = null;

let notificationUserId = null;
let notificationTaskId = null;
let historyRef = null;
let historyListener = null;

console.log("================================");
console.log("SDC Notification Server");
console.log("Starting...");
console.log("================================");

taskRef.on("value", async (snapshot) => {

    try {

        if (!snapshot.exists()) {

            console.log("Task = none");

            clearCleanupTimer();
            stopHistoryTracking();

            notificationUserId = null;
            notificationTaskId = null;

            return;
        }

        const task = snapshot.val();

        console.log("--------------------------------");
        console.log("Task detected");
        console.log(task);

        const taskId =
            task.task_id ||
            snapshot.key ||
            null;

        const userId =
            task.userid ||
            null;

        const notificationSend =
            task.notify_finish === true;

        if (
            notificationSend &&
            userId &&
            taskId
        ) {
            console.log("Get Noti")

            if (
                notificationUserId !== userId ||
                notificationTaskId !== taskId
            ) {

                notificationUserId = userId;
                notificationTaskId = taskId;

                console.log("--------------------------------");
                console.log("Notification tracking enabled");
                console.log("User ID =", notificationUserId);
                console.log("Task ID =", notificationTaskId);

                startHistoryTracking(
                    notificationUserId,
                    notificationTaskId
                );
            }

        } else {
            console.log("don't get nofi")

            if (
                notificationUserId !== null ||
                notificationTaskId !== null
            ) {

                stopHistoryTracking();

                notificationUserId = null;
                notificationTaskId = null;
            }
        }

        let startedAt =
            Number(task.started_at);

        let finishedAt =
            Number(task.finished_at);

        if (!startedAt || !finishedAt) {

            const duration =
                Number(task.duration);

            if (
                !Number.isFinite(duration) ||
                duration <= 0
            ) {

                console.log("Invalid task duration");

                return;
            }

            startedAt =
                Math.floor(Date.now() / 1000);

            finishedAt =
                startedAt + duration;

            await taskRef.update({
                started_at: startedAt,
                finished_at: finishedAt
            });

            console.log("Task time initialized");
            console.log(
                "started_at =",
                startedAt
            );

            console.log(
                "finished_at =",
                finishedAt
            );
        }

        scheduleTaskCleanup(
            finishedAt
        );

    } catch (error) {

        console.error(
            "Task listener error:",
            error
        );
    }
});

function startHistoryTracking(
    userId,
    taskId
) {

    stopHistoryTracking();

    historyRef =
        db.ref(`users/${userId}/history`);

    console.log("--------------------------------");
    console.log("Start history tracking");
    console.log(
        `users/${userId}/history`
    );

    historyListener =
        historyRef.on("child_added", async (snapshot) => {

            try {

                if (
                    snapshot.key !== taskId
                ) {

                    return;
                }

                const historyTask =
                    snapshot.val();

                console.log("--------------------------------");
                console.log("Matching history detected");
                console.log("Task ID =", snapshot.key);
                console.log(historyTask);

                await sendTaskNotification(
                    userId,
                    historyTask
                );

                stopHistoryTracking();

            } catch (error) {

                console.error(
                    "History listener error:",
                    error
                );
            }
        });
}

function stopHistoryTracking() {

    if (
        historyRef !== null &&
        historyListener !== null
    ) {

        historyRef.off(
            "child_added",
            historyListener
        );
    }

    historyRef = null;
    historyListener = null;
}

async function sendTaskNotification(
    userId,
    task
) {

    try {

        const tokenSnapshot =
            await db
                .ref(`users/${userId}/fcmToken`)
                .once("value");

        const fcmToken =
            tokenSnapshot.val();

        if (!fcmToken) {

            console.log(
                "FCM token not found"
            );

            return;
        }

        const duration =
            Number(task.duration);

        const durationText =
            formatDuration(duration);

        const message = {
            token: fcmToken,

            notification: {
                title: "งานฆ่าเชื้อเสร็จแล้ว",
                body: `ใช้เวลา ${durationText}`
            },

            data: {
                type: "task_completed",
                task_id:
                    task.task_id ||
                    "",
                duration:
                    String(duration)
            }
        };

        const response =
            await messaging.send(message);

        console.log("--------------------------------");
        console.log("Notification sent");
        console.log("User ID =", userId);
        console.log("Task ID =", task.task_id);
        console.log("Duration =", durationText);
        console.log("FCM response =", response);

    } catch (error) {

        console.error(
            "Notification send error:",
            error
        );
    }
}

function formatDuration(seconds) {

    seconds =
        Number(seconds) || 0;

    seconds =
        Math.max(
            0,
            Math.floor(seconds)
        );

    const hours =
        Math.floor(
            seconds / 3600
        );

    const minutes =
        Math.floor(
            (seconds % 3600) / 60
        );

    const remainingSeconds =
        seconds % 60;

    const parts = [];

    if (hours > 0) {

        parts.push(
            `${hours} ชั่วโมง`
        );
    }

    if (minutes > 0) {

        parts.push(
            `${minutes} นาที`
        );
    }

    if (
        remainingSeconds > 0 ||
        parts.length === 0
    ) {

        parts.push(
            `${remainingSeconds} วินาที`
        );
    }

    return parts.join(" ");
}

function scheduleTaskCleanup(
    finishedAt
) {

    clearCleanupTimer();

    const now =
        Math.floor(
            Date.now() / 1000
        );

    const deleteAt =
        finishedAt + 10;

    if (now >= deleteAt) {

        deleteExpiredTask();

        return;
    }

    const delay =
        (deleteAt - now) * 1000;

    console.log("--------------------------------");
    console.log("Task cleanup scheduled");
    console.log(
        "finished_at =",
        finishedAt
    );

    console.log(
        "delete_at =",
        deleteAt
    );

    console.log(
        "remaining =",
        deleteAt - now,
        "sec"
    );

    cleanupTimer =
        setTimeout(() => {

            deleteExpiredTask();

        }, delay);
}

async function deleteExpiredTask() {

    clearCleanupTimer();

    try {

        const snapshot =
            await taskRef.once("value");

        if (!snapshot.exists()) {

            return;
        }

        const task =
            snapshot.val();

        const finishedAt =
            Number(task.finished_at);

        const now =
            Math.floor(
                Date.now() / 1000
            );

        if (
            !Number.isFinite(
                finishedAt
            )
        ) {

            return;
        }

        if (
            now <
            finishedAt + 10
        ) {

            scheduleTaskCleanup(
                finishedAt
            );

            return;
        }

        const userId =
            task.userid;

        if (!userId) {

            console.log(
                "Task has no userid"
            );

            return;
        }

        const taskId =
            task.task_id ||
            `task_${Date.now()}`;

        task.status =
            "completed";

        task.started_at =
            formatThailandTime(
                Number(
                    task.started_at
                )
            );

        task.finished_at =
            formatThailandTime(
                Number(
                    task.finished_at
                )
            );

        const historyRef =
            db.ref(
                `users/${userId}/history/${taskId}`
            );

        await historyRef.set(
            task
        );

        await taskRef.remove();

        console.log("--------------------------------");
        console.log("Task completed");
        console.log("Task moved to history");

        console.log(
            `users/${userId}/history/${taskId}`
        );

        console.log(
            "Task deleted from device"
        );

    } catch (error) {

        console.error(
            "Task cleanup error:",
            error
        );

        scheduleTaskCleanup(
            Math.floor(
                Date.now() / 1000
            ) + 10
        );
    }
}

function formatThailandTime(
    timestamp
) {

    return new Date(
        timestamp * 1000
    )
        .toLocaleString(
            "sv-SE",
            {
                timeZone: "Asia/Bangkok",
                year: "numeric",
                month: "2-digit",
                day: "2-digit",
                hour: "2-digit",
                minute: "2-digit",
                second: "2-digit",
                hour12: false
            }
        )
        .replace(" ", "T") +
        "+07:00";
}

function clearCleanupTimer() {

    if (
        cleanupTimer !== null
    ) {

        clearTimeout(
            cleanupTimer
        );

        cleanupTimer = null;
    }
}