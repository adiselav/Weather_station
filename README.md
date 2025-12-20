# ESP32-C3 Weather Monitoring System

An IoT-based weather monitoring system utilizing ESP32-C3 microcontroller with multiple environmental sensors and a real-time web dashboard for data visualization.

## Hardware Components

- **ESP32-C3 Super Mini** - Main microcontroller
- **SHT21** - Temperature and humidity sensor
- **BMP085** - Barometric pressure sensor
- **SCD4x** - CO2 sensor with temperature and humidity measurements

## System Architecture

The system consists of three main components:

1. **Firmware** - ESP32-C3 embedded code for sensor data acquisition and transmission
2. **Gateway** - Node.js REST API server for data ingestion and MongoDB storage
3. **Dashboard** - Web-based visualization interface for real-time and historical data analysis

## Setup Instructions

### 1. Firmware Setup

1. Create a `config.h` file in the `/firmware/index` directory:

   ```cpp
   #define WIFI_SSID "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   #define SERVER_URL "http://YOUR_SERVER_IP:3000/api/readings"
   #define API_KEY "YOUR_SECURE_API_KEY"
   #define WIFI_RECONNECT_TIMEOUT 30000
   #define HTTP_MAX_RETRIES 3
   #define HTTP_RETRY_DELAY 5000
   ```

2. Upload the firmware to ESP32-C3 using Arduino IDE

3. Data transmission interval: 1 hour

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

2. Create a `.env` file:

   ```env
   MONGODB_URL=mongodb://localhost:27017/
   DB_NAME=weather_station
   COLLECTION_NAME=readings
   PORT=4000
   REFRESH_INTERVAL_SECONDS=30
   ```

3. Start the dashboard server:

   ```bash
   npm start
   ```

4. Access the dashboard at `http://localhost:4000`

## System Configuration

### Data Collection

- Measurement interval: 1 hour
- Data retention: Unlimited (MongoDB)
- Dashboard refresh: Configurable (default: 30 seconds)

## API Endpoints

### Gateway Server (Port 3000)

- `POST /api/readings` - Receive sensor data from ESP32
- `GET /api/readings` - Retrieve recent readings
- `GET /` - Health check endpoint

### Dashboard Server (Port 4000)

- `GET /api/readings/latest` - Get most recent reading
- `GET /api/readings/history` - Get historical data
- `GET /api/config` - Get dashboard configuration
- `GET /api/health` - Health check endpoint
- `GET /` - Web dashboard interface

## License

MIT
