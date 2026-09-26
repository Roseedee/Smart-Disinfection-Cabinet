const { initializeApp, cert } = require("firebase-admin/app");
const { getDatabase } = require("firebase-admin/database");

const serviceAccount = require("../serviceAccountKey.json");

initializeApp({
    credential: cert(serviceAccount),
    databaseURL: "https://esp-lab-c89db-default-rtdb.asia-southeast1.firebasedatabase.app"
});

const db = getDatabase();

const deviceSn = "AWE416E1W61";

const taskRef = db
    .ref("devices")
    .child(deviceSn)
    .child("task");

let currentTaskId = null;
let timer = null;

console.log("================================");
console.log("SDC ESP Test Service");
console.log(`Device: ${deviceSn}`);
console.log("Starting...");
console.log("================================");

function stopTimer() {
    if (timer !== null) {
        clearInterval(timer);
        timer = null;
    }
}

function getNumber(value, fallback = 0) {
    const number = Number(value);

    return Number.isFinite(number)
        ? number
        : fallback;
}

async function acceptTask(snapshot) {
    const task = snapshot.val();

    if (!task) {
        return;
    }

    const taskId = task.task_id || null;

    if (!taskId) {
        console.log("Task has no task_id");
        return;
    }

    currentTaskId = taskId;

    const command = task.command || "none";

    console.log("--------------------------------");
    console.log("New task received");
    console.log("Task ID:", taskId);
    console.log("Command:", command);

    await taskRef.update({
        status: "accepted"
    });

    if (command === "start") {
        await startTask();
        return;
    }

    if (command === "stop") {
        await stopTask();
        return;
    }

    if (command === "cancel") {
        await cancelTask();
        return;
    }

    await taskRef.child("command").set("none");

    console.log("Task accepted");
}

async function startTask() {
    if (!currentTaskId) {
        return;
    }

    if (timer !== null) {
        return;
    }

    const snapshot = await taskRef.once("value");

    if (!snapshot.exists()) {
        currentTaskId = null;
        return;
    }

    const task = snapshot.val();

    const taskId = task.task_id || null;

    if (taskId !== currentTaskId) {
        return;
    }

    const duration = getNumber(
        task.duration,
        0
    );

    let remaining = getNumber(
        task.remaining,
        duration
    );

    if (duration <= 0) {
        await taskRef.update({
            status: "completed",
            command: "none",
            remaining: 0,
            progress: 100
        });

        console.log("--------------------------------");
        console.log("Task completed");

        return;
    }

    if (remaining <= 0) {
        await taskRef.update({
            status: "completed",
            command: "none",
            remaining: 0,
            progress: 100
        });

        console.log("--------------------------------");
        console.log("Task completed");

        return;
    }

    await taskRef.update({
        status: "running",
        command: "none"
    });

    console.log("--------------------------------");
    console.log("Task started");
    console.log("Duration:", duration);
    console.log("Remaining:", remaining);

    timer = setInterval(async () => {
        try {
            remaining--;

            if (remaining <= 0) {
                remaining = 0;

                stopTimer();

                await taskRef.update({
                    status: "completed",
                    command: "none",
                    remaining: 0,
                    progress: 100
                });

                console.log("--------------------------------");
                console.log("Task completed");

                return;
            }

            const progress =
                ((duration - remaining) / duration) * 100;

            await taskRef.update({
                status: "running",
                command: "none",
                remaining: remaining,
                progress: Math.floor(progress)
            });

            console.log(
                `Task running | remaining: ${remaining}s | progress: ${Math.floor(progress)}%`
            );
        } catch (error) {
            console.error(
                "Timer error:",
                error
            );
        }
    }, 1000);
}

async function stopTask() {
    stopTimer();

    if (!currentTaskId) {
        return;
    }

    const snapshot =
        await taskRef.once("value");

    if (!snapshot.exists()) {
        currentTaskId = null;
        return;
    }

    await taskRef.update({
        status: "paused",
        command: "none"
    });

    console.log("--------------------------------");
    console.log("Task paused");
}

async function cancelTask() {
    stopTimer();

    if (!currentTaskId) {
        return;
    }

    const snapshot =
        await taskRef.once("value");

    if (!snapshot.exists()) {
        currentTaskId = null;
        return;
    }

    console.log("--------------------------------");
    console.log("Task cancelled");

    await taskRef.remove();

    currentTaskId = null;
}

taskRef.on("value", async (snapshot) => {
    if (!snapshot.exists()) {
        stopTimer();
        currentTaskId = null;
        return;
    }

    const task = snapshot.val();

    const taskId = task.task_id || null;
    const command = task.command || "none";
    const status = task.status || "none";

    if (!taskId) {
        return;
    }

    if (currentTaskId === null) {
        await acceptTask(snapshot);
        return;
    }

    if (taskId !== currentTaskId) {
        stopTimer();
        currentTaskId = null;
        await acceptTask(snapshot);
        return;
    }

    if (command === "start") {
        await startTask();
        return;
    }

    if (command === "stop") {
        await stopTask();
        return;
    }

    if (command === "cancel") {
        await cancelTask();
        return;
    }

    if (status === "completed") {
        stopTimer();
    }
});