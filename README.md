# ESP32-C3 Super Mini Weather Station

A complete weather monitoring system using ESP32-C3 Super Mini with multiple sensors and a web dashboard.

## Hardware Components

- **ESP32-C3 Super Mini** - Main microcontroller
- **SHT21** - Temperature and humidity sensor
- **BMP085** - Barometric pressure sensor (and backup temperature)
- **SCD4x** - CO₂, temperature, and humidity sensor

## System Architecture

The system consists of three main components:

1. **Firmware** (`/firmware`) - ESP32-C3 code that reads sensors and sends data
2. **Gateway** (`/gateway`) - Node.js API server that receives data from ESP32 and stores it in MongoDB
3. **Dashboard** (`/dashboard`) - Web interface to visualize the weather data

## Setup Instructions

### 1. Firmware Setup

1. Create a `config.h` file in the `/firmware` directory:

   ```cpp
   // config.h
   #define WIFI_SSID "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   #define SERVER_URL "http://YOUR_SERVER_IP:3000/api/readings"
   #define API_KEY "YOUR_SECURE_API_KEY"
   #define WIFI_RECONNECT_TIMEOUT 30000  // 30 seconds
   #define HTTP_MAX_RETRIES 3
   #define HTTP_RETRY_DELAY 5000  // 5 seconds
   ```

2. Upload the firmware to ESP32-C3 using Arduino IDE

3. The station will measure and send data **every 1 hour**

### 2. Gateway Server Setup

1. Navigate to the gateway directory:

   ```bash
   cd gateway
   npm install
   ```

2. Create a `.env` file:

   ```env
   MONGODB_URL=mongodb://localhost:27017/
   DB_NAME=weather_station
   COLLECTION_NAME=readings
   API_KEY=REPLACE_WITH_YOUR_SECURE_API_KEY
   PORT=3000
   ```

3. Start the gateway server:

   ```bash
   node index.js
   ```

### 3. Dashboard Setup

1. Navigate to the dashboard directory:

   ```bash
   cd dashboard
   npm install
   ```

2. Create a `.env` file (must match gateway settings):

   ```env
   MONGODB_URL=mongodb://localhost:27017/
   DB_NAME=weather_station
   COLLECTION_NAME=readings
   PORT=4000
   REFRESH_INTERVAL_SECONDS=30
   ```

   **Note:** All variables in `.env` are required. There are no default values. Copy `.env.example` to `.env` and fill in your values.

3. Start the dashboard server:

   **Production mode:**
   ```bash
   npm start
   ```

   **Development mode (with auto-restart):**
   ```bash
   npm run dev
   ```

   Development mode uses nodemon to automatically restart the server when you make changes to the code. This is useful for debugging without needing to manually redeploy to Render after each change.

4. Open your browser to: `http://localhost:4000`

   In development mode, the browser will open automatically.

## Testing Database Connection

If the dashboard shows no data, run the test script:

```bash
cd dashboard
node test-connection.js
```

This will verify:

- MongoDB connection
- Database and collection names
- Number of readings stored
- Latest reading details

## Troubleshooting

### Dashboard shows no data

1. **Check if data exists in MongoDB:**

   ```bash
   cd dashboard
   node test-connection.js
   ```

2. **Verify database names match:**

   - Gateway and Dashboard must use the same `DB_NAME` and `COLLECTION_NAME`
   - Check your `.env` files in both `/gateway` and `/dashboard`

3. **Check server console output:**
   - The dashboard server logs how many readings were found on startup
   - Check for any error messages

### ESP32 not sending data

1. **Check Serial Monitor:**
   - Look for WiFi connection status
   - Check HTTP response codes
   - Verify sensor initialization messages

2. **Verify network settings:**
   - Ensure `SERVER_URL` points to the correct IP and port
   - Check that `API_KEY` matches between firmware and gateway

3. **Test gateway endpoint:**

   ```bash
   curl http://YOUR_SERVER_IP:3000/
   ```

   Should return: `{"status":"ok","message":"Weather Station Server",...}`

## Development Tips

### Local Development vs Production

For local debugging and development:

- Use `npm run dev` in the dashboard folder to enable auto-restart on file changes
- The browser will open automatically at `http://localhost:4000`
- No need to manually redeploy to Render after each change
- Changes to server.js or other files will trigger an automatic restart

For production deployment on Render:

- Use `npm start` which runs the server without nodemon
- Environment variables are managed through Render's dashboard

## Data Collection Schedule

- **Measurement interval:** 1 hour (3600000 ms)
- **Data retention:** Unlimited (stored in MongoDB)
- **Dashboard refresh:** Configurable via `REFRESH_INTERVAL_SECONDS` in `.env` (default example: 30 seconds) + manual refresh button

## API Endpoints

### Gateway Server (Port 3000)

- `POST /api/readings` - Receive data from ESP32 (requires API key)
- `GET /api/readings` - View last 20 readings
- `GET /` - Health check

### Dashboard Server (Port 4000)

- `GET /api/readings/latest` - Get latest reading
- `GET /api/readings/history` - Get historical data (all readings, or use `?limit=N` for specific count)
- `GET /api/config` - Get dashboard configuration (refresh interval)
- `GET /api/health` - Health check
- `GET /` - Web dashboard (HTML)

## Available Commands

### Dashboard
- `npm start` - Start server in production mode
- `npm run dev` - Start server in development mode with auto-restart (nodemon)

## License

MIT
