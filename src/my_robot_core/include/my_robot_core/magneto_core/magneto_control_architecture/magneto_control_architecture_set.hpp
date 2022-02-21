#include <my_robot_core/magneto_core/magneto_control_architecture/mpc_architecture.hpp>
#include <my_robot_core/magneto_core/magneto_control_architecture/wbmc_architecture.hpp>

namespace MAGNETO_STATES {
constexpr int INITIALIZE = 0;
constexpr int BALANCE = 1; // DEFAULT
constexpr int SWING_START_TRANS = 2;
constexpr int SWING = 3;
constexpr int SWING_END_TRANS = 4;
};  // namespace MAGNETO_STATES

class SimMotionCommand;
class MagnetoUserStateCommand {
   public:
   MagnetoUserStateCommand(){
       state_id = -1;
       user_cmd = SimMotionCommand();
   }
   void setCommand(int _state_id, const SimMotionCommand& _state_cmd) {
       state_id = _state_id;
       user_cmd = _state_cmd;
   }
   int state_id;
   SimMotionCommand user_cmd;
};