// initializes all the subsystems we desire
[[nodiscard]]  bool system_initialize();

// handles restarts
void system_restart();

// will be called before esp_reset (if possible)
// flush message if possible to the disk
// reset GPIO if possible etc.
void system_shutdown();