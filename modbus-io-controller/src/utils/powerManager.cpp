#include "powerManager.h"

uint32_t powerTS = 0;

void init_powerManager(void) {
  analogReadResolution(12);
  log(LOG_INFO, false, "Power monitoring task started\n");
}

void managePower(void) {
  if (millis() - powerTS < POWER_UPDATE_INTERVAL) return;
  powerTS += POWER_UPDATE_INTERVAL;
  static float Vpsu;
  static bool psuOK;
  bool statusChanged = false;

  // Read voltage values
  Vpsu = 0.0;
  for (int i = 0; i < 10; i++) {
    Vpsu += (float)analogRead(PIN_PSU_FB) * V_PSU_MUL_V;
  }
  Vpsu /= 10.0;

  // Check PSU voltage
  if ((Vpsu > V_PSU_MAX || Vpsu < V_PSU_MIN) && psuOK) {
    log(LOG_WARNING, false, "PSU voltage out of range: %.2f V\n", Vpsu);
    statusChanged = true;
    psuOK = false;
  } else if (Vpsu <= V_PSU_MAX && Vpsu >= V_PSU_MIN && !psuOK) {
    log(LOG_INFO, false, "PSU voltage OK: %.2f V\n", Vpsu);
    statusChanged = true;
    psuOK = true;
  }

  // Update global status
  if (statusLocked) return;
  statusLocked = true;
  status.Vpsu = Vpsu;
  if (statusChanged) {
    status.psuOK = psuOK;
  }
  status.updated = true;
  statusLocked = false;
}