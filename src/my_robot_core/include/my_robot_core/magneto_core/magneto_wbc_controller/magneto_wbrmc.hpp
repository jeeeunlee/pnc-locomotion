#pragma once

// #include <my_robot_core/magneto_core/magneto_definition.hpp>
// #include <my_robot_core/magneto_core/magneto_interface.hpp>
// #include <my_robot_core/magneto_core/magneto_state_provider.hpp>
// #include <my_robot_core/magneto_core/magneto_wbc_controller/containers/wbc_spec_container.hpp>
// #include <my_wbc/JointIntegrator.hpp>
// #include <my_wbc/WBLC/KinWBC.hpp>
#include <my_wbc/WBMC/MCWBC.hpp>
#include <my_robot_core/magneto_core/magneto_wbc_controller/magneto_wbmc.hpp>

class MagnetoWBRMC: public MagnetoWBMC {
 public:
  MagnetoWBRMC(MagnetoWbcSpecContainer* _ws_container,
                            RobotSystem* _robot);
  virtual ~MagnetoWBRMC();

  virtual void getCommand(void* _cmd);
  virtual void ctrlInitialization(const YAML::Node& node);

 protected:
  //  Processing Step for first visit
  virtual void firstVisit();  

  // Redefine PreProcessing Command
  virtual void _PreProcessing_Command();

 private:
 // Controller Objects
  // WBRMC* wbrmc_;
  // WBRMC_ExtraData* wbrmc_param_;

  MCWBC* wbrmc_;
  MCWBC_ExtraData* wbrmc_param_;
  
  
};
