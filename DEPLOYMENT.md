# Deployment pe Render - Ghid Complet

Acest ghid te ajută să hostezi gateway-ul și dashboard-ul pe Render.

## 📋 Pași de Deployment

### 1. Pregătire Repository

Asigură-te că ai:
- ✅ Codul push-at pe GitHub/GitLab/Bitbucket
- ✅ MongoDB Atlas configurat (sau MongoDB local)
- ✅ Toate dependențele în `package.json`

### 2. Creare Servicii pe Render

#### Opțiunea A: Deployment Automat cu render.yaml (Recomandat)

1. **Conectează repository-ul pe Render:**
   - Mergi pe [render.com](https://render.com)
   - Click pe "New" → "Blueprint"
   - Conectează repository-ul tău
   - Render va detecta automat `render.yaml`

2. **Configurare Variabile de Mediu:**
   Render va cere următoarele variabile (sau le poți seta manual în dashboard):

   **Pentru Gateway:**
   - `MONGODB_URL` - Connection string MongoDB (ex: `mongodb+srv://user:pass@cluster.mongodb.net/dbname`)
   - `DB_NAME` - `weather_station` (sau lasă default)
   - `COLLECTION_NAME` - `readings` (sau lasă default)
   - `API_KEY` - Cheia ta secretă pentru autentificare
   - `PORT` - Setat automat de Render (nu trebuie setat manual)

   **Pentru Dashboard:**
   - `MONGODB_URL` - Același ca la Gateway
   - `DB_NAME` - `weather_station` (trebuie să fie același ca Gateway)
   - `COLLECTION_NAME` - `readings` (trebuie să fie același ca Gateway)
   - `REFRESH_INTERVAL_SECONDS` - `30` (sau valoarea dorită)
   - `PORT` - Setat automat de Render

#### Opțiunea B: Deployment Manual

**Gateway Service:**
1. Click "New" → "Web Service"
2. Conectează repository-ul
3. Setează:
   - **Name:** `weather-station-gateway`
   - **Root Directory:** `gateway`
   - **Environment:** `Node`
   - **Build Command:** `npm install`
   - **Start Command:** `npm start`
4. Adaugă variabilele de mediu (vezi mai sus)

**Dashboard Service:**
1. Click "New" → "Web Service"
2. Conectează repository-ul
3. Setează:
   - **Name:** `weather-station-dashboard`
   - **Root Directory:** `dashboard`
   - **Environment:** `Node`
   - **Build Command:** `npm install`
   - **Start Command:** `npm start`
4. Adaugă variabilele de mediu (vezi mai sus)

### 3. Actualizare Configurație ESP32

După ce serviciile sunt live, actualizează `firmware/index/config.h`:

```cpp
// Înlocuiește cu URL-ul gateway-ului de pe Render
const char* SERVER_URL = "https://weather-station-gateway.onrender.com/api/readings";

// API_KEY trebuie să fie același ca cel setat în Render
const char* API_KEY = "api_wxJAMcFVioe4kcuasrdpDIjrQPq1bWcS";
```

**Notă:** Render oferă URL-uri gratuite de tipul: `https://your-service-name.onrender.com`

### 4. Verificare Deployment

**Test Gateway:**
```bash
curl https://your-gateway-url.onrender.com/
# Ar trebui să returneze: {"status":"ok","message":"Weather Station Server",...}
```

**Test Dashboard:**
- Deschide în browser: `https://your-dashboard-url.onrender.com`
- Ar trebui să vezi dashboard-ul

### 5. Configurare Auto-Deploy (Opțional)

Render deployează automat la fiecare push pe branch-ul principal. Poți configura:
- Branch-ul pentru deployment
- Auto-deploy on/off
- Preview deployments pentru pull requests

## 🔒 Securitate

1. **API Key:** Folosește o cheie puternică și o păstrează secretă
2. **MongoDB:** Asigură-te că connection string-ul este securizat
3. **HTTPS:** Render oferă HTTPS automat pentru toate serviciile

## 📝 Variabile de Mediu - Checklist

### Gateway
- [ ] `MONGODB_URL` - Connection string MongoDB
- [ ] `DB_NAME` - Numele bazei de date
- [ ] `COLLECTION_NAME` - Numele colecției
- [ ] `API_KEY` - Cheia de autentificare
- [ ] `PORT` - Setat automat (nu seta manual)

### Dashboard
- [ ] `MONGODB_URL` - Același ca Gateway
- [ ] `DB_NAME` - Același ca Gateway
- [ ] `COLLECTION_NAME` - Același ca Gateway
- [ ] `REFRESH_INTERVAL_SECONDS` - Interval refresh (ex: 30)
- [ ] `PORT` - Setat automat (nu seta manual)

## 🐛 Troubleshooting

### Gateway nu pornește
- Verifică logs în Render dashboard
- Verifică că toate variabilele de mediu sunt setate
- Verifică că MongoDB connection string este corect

### Dashboard nu se conectează la MongoDB
- Verifică că `MONGODB_URL` este același în ambele servicii
- Verifică că `DB_NAME` și `COLLECTION_NAME` sunt identice

### ESP32 nu trimite date
- Verifică că `SERVER_URL` în `config.h` este corect
- Verifică că `API_KEY` este același în ESP32 și Gateway
- Verifică logs-urile gateway-ului pe Render

## 📚 Resurse

- [Render Documentation](https://render.com/docs)
- [MongoDB Atlas Setup](https://www.mongodb.com/cloud/atlas)

