// ============================================================
//   config.example.h  —  PLANTILLA DE CONFIGURACIÓN
// ============================================================
//
//   Esta es una PLANTILLA de ejemplo (sin datos reales).
//
//   Para que el proyecto funcione:
//     1) Copiá este archivo y renombralo a  config.h
//     2) Completá tus datos reales adentro de config.h
//
//   El archivo config.h con tus datos NO se sube a GitHub.
// ============================================================

#ifndef CONFIG_H
#define CONFIG_H

// --- 1) Red WiFi ---
#define WIFI_SSID      "NOMBRE_DE_TU_WIFI"
#define WIFI_PASSWORD  "CLAVE_DE_TU_WIFI"

// --- 2) Bot de Telegram (te lo da @BotFather) ---
#define BOT_TOKEN      "PEGA_ACA_EL_TOKEN_DE_TU_BOT"

// --- 3) Chat autorizado (opcional; vacío = cualquiera puede usarlo) ---
#define CHAT_ID_AUTORIZADO  ""

#endif  // CONFIG_H
