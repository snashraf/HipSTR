#include "stutter_model.h"

std::ostream& operator<< (std::ostream &out, StutterModel& model){
  out << "IN_FRAME [P_GEOM(rep)=" << model.in_geom_  << ", P_DOWN=" << model.in_down_  << ", P_UP=" << model.in_up_  << "]" << "\n"
      << "OUT_FRAME[P_GEOM(bp) =" << model.out_geom_ << ", P_DOWN=" << model.out_down_ << ", P_UP=" << model.out_up_ << "]" << std::endl;
  return out;
}

double StutterModel::get_parameter(bool in_frame, char parameter){
  switch(parameter){
  case 'U':
    return in_frame ? in_up_   : out_up_;
  case 'D':
    return in_frame ? in_down_ : out_down_;
  case 'P':
      return in_frame ? in_geom_ : out_geom_;
  default:
    printErrorAndDie("Invalid such stutter model parameter requested");
    return -1.0;
  }
}


/*
  Returns the read's log-likeliood given that it contains exactly the provided number of base pairs
*/
double StutterModel::log_stutter_pmf(int sample_bps, int read_bps){
  double log_pmf;
  int bp_diff = read_bps - sample_bps;
  if (bp_diff % motif_len_ != 0){
    int eff_diff = bp_diff - (bp_diff/motif_len_);
    if (eff_diff < 0)
      log_pmf = out_log_down_ + out_log_nostep_ + out_log_step_*(-eff_diff-1);
    else
      log_pmf = out_log_up_   + out_log_nostep_ + out_log_step_*(eff_diff-1);
  }
  else {
    int rep_diff = bp_diff/motif_len_;
    if (rep_diff == 0)
      log_pmf = log_equal_;
    else {
      if (rep_diff < 0)
	log_pmf = in_log_down_ + in_log_nostep_ + in_log_step_*(-rep_diff-1);
      else
	log_pmf = in_log_up_   + in_log_nostep_ + in_log_step_*(rep_diff-1);
    }
  }
  assert(log_pmf <= 0);
  return log_pmf;
}


/*
  Returns the read's log-likelihood given that it contains at least the provided number of base pairs
*/
double StutterModel::log_stutter_geq(int sample_bps, int min_read_bps){
  std::vector<double> log_probs;
  int min_bp_diff = min_read_bps - sample_bps;
  
  // Incorporate all potential in-frame stutters
  int next_rep_diff = (min_bp_diff < 0 || min_bp_diff % motif_len_ == 0 ? min_bp_diff/motif_len_:  1 + min_bp_diff/motif_len_);
  if (next_rep_diff < 0){
    log_probs.push_back(in_log_down_ + log_geom_leq(in_geom_, -next_rep_diff));
    log_probs.push_back(log_equal_);
    log_probs.push_back(in_log_up_);    
  }
  else if (next_rep_diff == 0){
    log_probs.push_back(log_equal_);
    log_probs.push_back(in_log_up_);
  }
  else
    log_probs.push_back(in_log_up_ + log_geom_geq(in_geom_, next_rep_diff));
  
  // Incorporate all potential out-of-frame stutters
  int next_outframe_diff = min_bp_diff + (min_bp_diff % motif_len_ == 0);
  int eff_diff = next_outframe_diff - (next_outframe_diff/motif_len_);
  if (eff_diff < 0){
    log_probs.push_back(out_log_down_ + log_geom_leq(out_geom_, -eff_diff));
    log_probs.push_back(out_log_up_);
  }
  else
    log_probs.push_back(out_log_up_ + log_geom_geq(out_geom_, eff_diff));
  
  return log_sum_exp(log_probs);
}
