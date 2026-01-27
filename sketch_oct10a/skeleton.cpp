#include "skeleton.h"
#include <utility>
#include <FS.h> 
#include <SPIFFS.h>

Skeleton::Skeleton() {
  uint64_t esp = ESP.getEfuseMac();  // 1️⃣ Получаем уникальный MAC в 64 бит
  id = String((uint64_t)esp);
}


