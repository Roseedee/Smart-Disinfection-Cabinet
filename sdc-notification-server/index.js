const { initializeApp, cert } = require("firebase-admin/app");

const { getDatabase } = require("firebase-admin/database");

const serviceAccount = require("./serviceAccountKey.json");

initializeApp({
    credential: cert(serviceAccount),
    databaseURL: "https://esp-lab-c89db-default-rtdb.asia-southeast1.firebasedatabase.app"
});

const db = getDatabase();

const deviceRef =
    db.ref("devices/AWE416E1W61");

console.log("================================");
console.log("SDC Notification Server");
console.log("Starting...");
console.log("================================");

deviceRef.on("value", (snapshot) => {

    const device = snapshot.val();

    console.log("--------------------------------");
    console.log("Device data changed");
    console.log(device);
});