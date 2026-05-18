# Sistema-de-Riego-EQ1-LIDTS
# 🌱 Sistema de Riego Inteligente con IoT — ESP32 + Blynk

> Proyecto final de la materia **Arquitectura de Computadoras**  
---

## 📱 Configuración del dashboard en Blynk

### Pasos iniciales
1. Crear cuenta en [blynk.cloud](https://blynk.cloud)
2. Crear nuevo **Template** con nombre `Regadero automatico`
3. Crear un nuevo **Device** y copiar el Auth Token a `secrets.h`

### Widgets del dashboard

Recrear los siguientes widgets en la app Blynk:

| Pin Virtual | Widget | Tipo | Rango | Descripción |
|---|---|---|---|---|
| **V1** | Gauge / Label | Lectura | 0–100 | Humedad del suelo (%) |
| **V2** | Gauge / Label | Lectura | -10–60 | Temperatura ambiente (°C) |
| **V3** | Gauge / Label | Lectura | 0–100 | Humedad ambiental (%) |
| **V4** | Gauge / Label | Lectura | 0–100 | Nivel de agua en depósito (%) |
| **V5** | Button | Escritura | 0/1 | Control manual de la bomba (ON/OFF) |
| **V6** | LED / Notification | Lectura | 0/1 | Alarma de sequía |
| **V7** | Switch | Escritura | 0/1 | Modo AUTO (1) / MANUAL (0) |

### Evento de notificación
Crear un evento llamado `sequia` en la sección **Events** del Template. Se disparará automáticamente cuando la humedad del suelo baje del 40%.
