# Coding Rules

## 1. Comments: English only

## 2. Brace Style: Same line
```c
void func() {
}
if (x) {
}
```

## 3. Architecture: 3-Layer
- **app** → calls **drv** only
- **drv** → calls **bsp** only  
- **lib** → callable by all layers
- ❌ app cannot call bsp directly

## 4. File Naming & standard
- All new files must use layer prefix: `app_`, `bsp_`, `drv_`, or `lib_`
- All new files must refer to standard file .c and .h (see `example.c` and `example.h`)

## 5. Task Creation Rules
When creating a new FreeRTOS task:
- **Task configuration** (priority, stack size, tick period) must be defined in `app_system.h`
- **Task function** (e.g., `Xxx_Task()`) must be implemented in `app_system.c`
- **Manager function** (e.g., `XxxManager()`) must be implemented in `app_xxx.c`
- **Task creation** must be done in `System_CreateTasks()` in `app_system.c`

## 6. Platform-Specific Code
- If code is specific to **STM32**, **GD32**, or **ESP32**, keep it and use conditional compilation
- Platform selection is controlled by macros defined in `app_system.h`:
  - `PLATFORM_STM32` for STM32
  - `PLATFORM_GD32` for GD32
  - `PLATFORM_ESP32` for ESP32
- Example usage:
```c
#ifdef PLATFORM_STM32
    // STM32-specific code
#elif defined(PLATFORM_GD32)
    // GD32-specific code
#elif defined(PLATFORM_ESP32)
    // ESP32-specific code
#endif
```


