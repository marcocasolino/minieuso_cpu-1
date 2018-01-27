#ifndef _CAM_MANAGER_H
#define _CAM_MANAGER_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <thread>
#include <future>
#include <chrono>

#include "log.h"
#include "CpuTools.h"

#define CAMERA_DIR "/home/software/CPU/cameras/multiplecam"
#define CAMERA_EXEC "(cd /home/software/CPU/cameras/multiplecam && ./multiplecam.sh)"
#define TEST "(cd /home/software/CPU/test && ./print_error)"
#define N_TRY_RELAUNCH 2
#define WAIT_TIME 120

class CamManager {
public:
  std::thread::native_handle_type cam_thread_handle;
  int n_relaunch_attempt;
  std::promise<bool> launch_failed;
  bool launch_running;
  
  CamManager();
  int SetVerbose();
  int KillCamAcq();
  int CollectData();
  
private:
  bool verbose = false;
  int StartAcquisition();

};

#endif
/* _CAM_MANAGER_H */
