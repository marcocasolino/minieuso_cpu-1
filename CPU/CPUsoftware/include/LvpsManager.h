#ifndef _LVPS_MANAGER_H
#define _LVPS_MANAGER_H

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#ifndef __APPLE__
#include <error.h>
#include "aDIO_library.h"
#endif /* __APPLE__ */
#include "log.h"

#define ONE_MILLISEC 1000

/* ON/OFF signal bit addresses */
#define HK_PORT_OFF 0x01 /* P0.0 - 0000 0001 */
#define HK_PORT_ON 0x02 /* P0.1  - 0000 0010 */
#define ZYNQ_PORT_OFF 0x04 /* P0.2  - 0000 0100 */
#define ZYNQ_PORT_ON 0x08 /* P0.3  - 0000 1000 */
#define CAMERA_PORT_OFF 0x10 /* P0.4  - 0001 0000 */
#define CAMERA_PORT_ON 0x20 /* P0.5  - 0010 0000 */

#define CC_LVPS_HK 0x10
#define RET_CC_LVPS_HK 0x20

#define HIGH_VAL 0xFF
#define LOW_VAL 0x00

class LvpsManager {
public:
  enum Status : uint8_t {
    OFF = 0,
    ON = 1,
    UNDEF = 2,
  };
  enum SubSystem : uint8_t {
    ZYNQ = 0,
    CAMERAS = 1,
    HK = 2,
  };

  Status zynq_status;
  Status cam_status;
  Status hk_status;
  
  LvpsManager();
  Status GetStatus(SubSystem sub_system);
  int SwitchOn(SubSystem sub_system);
  int SwitchOff(SubSystem sub_system);
  static int Check(SubSystem sub_system);

private:
  static const uint32_t minor_number = 0;
#ifndef __APPLE__
  DeviceHandle aDIO_Device;
#endif /* __APPLE__ */
  enum PortValue : uint8_t {
    HIGH = 0xFF,
    LOW = 0x00,
  };
  
  int InitPorts();
  int CloseDev();
  int SetDirP0(uint8_t port_config);
  int SetValP0(PortValue port_value);
  int SetPulseP0(uint8_t port_config);

};


#endif /* LVPS_MANAGER */
