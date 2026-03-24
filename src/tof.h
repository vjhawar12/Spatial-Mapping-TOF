#ifndef TOF_H
#define TOF_H

void VL53L1X_XSHUT(void);
void tof_init(void);
void tof_scan(void);
void tof_stop(void);
int tof_get_distance(void);
void tof_get_distance_nonblocking(void);

#endif