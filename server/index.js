const express = require('express');
const mongoose = require('mongoose');
const bodyParser = require('body-parser');
const cors = require('cors');

// ---------------- CONFIG ----------------

const MONGO_URL = "mongodb+srv://paduraru_andrei-dragos:stud@mongodb.ju0c87f.mongodb.net/meteo_station?retryWrites=true&w=majority";
const PORT = 3000;

// ---------------- SETUP ----------------

const app = express();
app.use(cors());
app.use(bodyParser.json());

// Conectare MongoDB
mongoose.connect(MONGO_URL)
    .then(() => {
        console.log("MongoDB connected");
    })
    .catch(err => {
        console.error("MongoDB connection error:", err.message);
    });

// Schema pentru măsurători
const ReadingSchema = new mongoose.Schema({
    temperature: Number,
    humidity: Number,
    pressure: Number,
    co2: Number,
    timestamp: { type: Date, default: Date.now }
}, {
    versionKey: false  // Dezactivează câmpul __v
});

const Reading = mongoose.model("Reading", ReadingSchema, "readings");

// ---------------- ROUTES ----------------

// POST /api/readings ← aici trimite ESP32
app.post("/api/readings", async (req, res) => {
    try {
        if (mongoose.connection.readyState !== 1) {
            return res.status(503).json({ 
                status: "error", 
                message: "MongoDB not connected" 
            });
        }

        const reading = new Reading({
            temperature: Number(req.body.temperature !== null && req.body.temperature !== undefined ? req.body.temperature : 0),
            humidity: Number(req.body.humidity !== null && req.body.humidity !== undefined ? req.body.humidity : 0),
            pressure: Number(req.body.pressure !== null && req.body.pressure !== undefined ? req.body.pressure : 0),
            co2: Number(req.body.co2 !== null && req.body.co2 !== undefined ? req.body.co2 : 0),
            timestamp: new Date()
        });
        
        await reading.save();
        res.json({ status: "ok" });
    } catch (err) {
        console.error("Error saving data:", err.message);
        res.status(500).json({ status: "error", message: err.message });
    }
});

// Endpoint opțional pentru verificare
app.get("/api/readings", async (req, res) => {
    const data = await Reading.find().sort({ timestamp: -1 }).limit(20);
    res.json(data);
});

// Endpoint pentru health check
app.get("/", (req, res) => {
    res.json({ 
        status: "ok", 
        message: "Weather Station Server",
        mongodb: mongoose.connection.readyState === 1 ? "connected" : "disconnected"
    });
});

// ---------------- START SERVER ----------------
app.listen(PORT, "0.0.0.0", () => {
    console.log(`Server running on port ${PORT}`);
});
