#include <my_robot_core/magneto_core/magneto_wbc_controller/magneto_wbmc.hpp>

MagnetoWBMC::MagnetoWBMC(
    MagnetoWbcSpecContainer* _ws_container, RobotSystem* _robot) {
  my_utils::pretty_constructor(2, "Magneto Main Controller");
  // Initialize Flag
  b_first_visit_ = true;

  // Initialize Pointer to the WBC Spec Obj 
  // (Contact, Task, Force and Weights) Container
  ws_container_ = _ws_container;
  robot_ = _robot;

  // Initialize State Provider
  sp_ = MagnetoStateProvider::getStateProvider(robot_);

  // Initialize Actuator selection list
  
  act_list_.resize(Magneto::n_dof, true);
  for (int i(0); i < Magneto::n_vdof; ++i) 
      act_list_[Magneto::idx_vdof[i]] = false;

  // Initialize WBC
  kin_wbc_ = new KinWBC(act_list_);
  wbmc_ = new WBMC(act_list_);
  wbmc_param_ = new WBMC_ExtraData();

  tau_cmd_ = Eigen::VectorXd::Zero(Magneto::n_adof);
  qddot_cmd_ = Eigen::VectorXd::Zero(Magneto::n_adof);

  // Initialize desired pos, vel, acc containers
  jpos_des_ = Eigen::VectorXd::Zero(Magneto::n_adof);
  jvel_des_ = Eigen::VectorXd::Zero(Magneto::n_adof);
  jacc_des_ = Eigen::VectorXd::Zero(Magneto::n_adof);
}

MagnetoWBMC::~MagnetoWBMC() {
  delete wbmc_;
}

void MagnetoWBMC::_PreProcessing_Command() {
  // Update Dynamic Terms
  A_ = robot_->getMassMatrix();
  Ainv_ = robot_->getInvMassMatrix();
  grav_ = robot_->getGravity();
  coriolis_ = robot_->getCoriolis();

  // Grab Variables from the container.
  wbmc_param_->W_qddot_ = ws_container_->W_qddot_;
  wbmc_param_->W_xddot_ = ws_container_->W_xddot_;
  wbmc_param_->W_rf_ = ws_container_->W_rf_;
  wbmc_param_->F_magnetic_ = ws_container_->F_magnetic_;

  // Clear out local pointers
  task_list_.clear();
  contact_list_.clear();

  // Update task and contact list pointers from container object
  for (int i = 0; i < ws_container_->task_list_.size(); i++) {
    task_list_.push_back(ws_container_->task_list_[i]);
  }
  for (int i = 0; i < ws_container_->contact_list_.size(); i++) {
    contact_list_.push_back(ws_container_->contact_list_[i]);
  }

  // Update Contact Spec
  for (int i = 0; i < contact_list_.size(); i++) {
    contact_list_[i]->updateContactSpec();
  }
}

void MagnetoWBMC::getCommand(void* _cmd) {
  // grab & update task_list and contact_list & QP weights
  _PreProcessing_Command();

  // ---- Solve Inv Kinematics
  kin_wbc_->FindConfiguration(sp_->q, task_list_, contact_list_, 
                                jpos_des_, jvel_des_, jacc_des_); 
  
  // my_utils::pretty_print(jpos_des_, std::cout, "jpos_des_");
  // my_utils::pretty_print(jvel_des_, std::cout, "jvel_des_");
  // my_utils::pretty_print(jacc_des_, std::cout, "jacc_des_");

  Eigen::VectorXd jacc_des_cmd =
      jacc_des_ +
      Kp_.cwiseProduct(jpos_des_ - sp_->getActiveJointValue()) +
      Kd_.cwiseProduct(jvel_des_ - sp_->getActiveJointValue(sp_->qdot));

  // my_utils::pretty_print(jacc_des_cmd, std::cout, "jacc_des_cmd");
                                
  // wbmc
  wbmc_->updateSetting(A_, Ainv_, coriolis_, grav_);
  wbmc_->makeTorqueGivenRef(jacc_des_cmd, contact_list_, jtrq_des_, wbmc_param_);

  for (int i(0); i < Magneto::n_adof; ++i) {
      ((MagnetoCommand*)_cmd)->jtrq[i] = jtrq_des_[i];
      ((MagnetoCommand*)_cmd)->q[i] = jpos_des_[i];
      ((MagnetoCommand*)_cmd)->qdot[i] = jvel_des_[i];
  }

  ws_container_->update_magnetism_map(((MagnetoCommand*)_cmd)->b_magnetism_map );


  // _PostProcessing_Command(); // unset task and contact

  // my_utils::pretty_print(((MagnetoCommand*)_cmd)->jtrq, std::cout, "jtrq");
  // my_utils::pretty_print(((MagnetoCommand*)_cmd)->q, std::cout, "q");
  // my_utils::pretty_print(((MagnetoCommand*)_cmd)->qdot, std::cout, "qdot");
}


void MagnetoWBMC::firstVisit() { 
  
}

void MagnetoWBMC::ctrlInitialization(const YAML::Node& node) {
  // WBC Defaults
  wbc_dt_ = MagnetoAux::servo_rate;
  b_enable_torque_limits_ = true;  // Enable WBC torque limits

  // Joint Integrator Defaults
  vel_freq_cutoff_ = 2.0;  // Hz
  pos_freq_cutoff_ = 1.0;  // Hz
  max_pos_error_ = 0.2;    // Radians

  // Load Custom Parmams ----------------------------------
  try {
    // Load Integration Parameters
    my_utils::readParameter(node, "enable_torque_limits", b_enable_torque_limits_);
    my_utils::readParameter(node, "torque_limit", torque_limit_);    
    
    my_utils::readParameter(node, "velocity_freq_cutoff", vel_freq_cutoff_);
    my_utils::readParameter(node, "position_freq_cutoff", pos_freq_cutoff_);
    my_utils::readParameter(node, "max_position_error", max_pos_error_);

    my_utils::readParameter(node, "kp", Kp_);
    my_utils::readParameter(node, "kd", Kd_);

  } catch (std::runtime_error& e) {
    std::cout << "Error reading parameter [" << e.what() << "] at file: ["
              << __FILE__ << "]" << std::endl
              << std::endl;
    exit(0);
  }
  // ----------------------------------

  // Set WBC Parameters
  // Enable Torque Limits

  tau_min_ =
      // sp_->getActiveJointValue(robot_->GetTorqueLowerLimits());
      Eigen::VectorXd::Constant(Magneto::n_adof, -torque_limit_); //-2500.
  tau_max_ =
      // sp_->getActiveJointValue(robot_->GetTorqueUpperLimits());
      Eigen::VectorXd::Constant(Magneto::n_adof, torque_limit_); //-2500.
  wbmc_->setTorqueLimits(tau_min_, tau_max_);

  // Set Joint Integrator Parameters
}
