#include <memory>
#include "utils/include/serialize.h"
#include "math/include/constants.h"
#include "math/include/utils_math.h"
#include "math/include/table.h"
#include "configuration/include/domain.h"
#include "configuration/include/configuration.h"
#include "system/include/potential.h"
#include "system/include/model_empty.h"
#include "system/include/model_two_body.h"
#include "system/include/model_two_body_table.h"

namespace feasst {

Potential::Potential(argtype * args) {
  // parse group_index
  if (used("group_index", *args)) {
    group_index_ = integer("group_index", args, 0);
    ASSERT(!used("group", *args), "group and group_index cannot both be used");
  } else {
    group_index_ = 0;
    group_ = str("group", args, "");
  }
  if (used("cell_index", *args)) {
    ASSERT(group_index_ == 0 && group_.empty(),
      "cell_index overrides group_index");
    group_index_ = integer("cell_index", args);
  }
  prevent_cache_ = boolean("prevent_cache", args, false);
  table_size_ = integer("table_size", args, 0);

  // override args
  DEBUG("parsing model params");
  for (std::map<std::string, std::shared_ptr<ModelParam>>::iterator iter = ModelParam().deserialize_map().begin(); iter != ModelParam().deserialize_map().end(); ++iter) {
    const std::string param = iter->first;
    // loop through map arg keys
    for (auto it = args->cbegin(); it != args->cend();) {
      const std::string key = it->first;
      // if map key begins with param, add it to override_args_
      const int size = static_cast<int>(param.length());
      DEBUG("size " << size);
      DEBUG("key " << key);
      DEBUG("param " << param);
      if (key.substr(0, size) == param) {
        DEBUG("** extracting sigma");
        override_args_.insert({key, str(key, *args)});
        it = args->erase(it);
      } else {
        ++it;
      }
    }
  }
}

Potential::Potential(std::shared_ptr<Model> model,
                     argtype args) : Potential(&args) {
  model_ = model;
  visit_model_ = std::make_shared<VisitModel>();
  FEASST_CHECK_ALL_USED(args);
}

Potential::Potential(std::shared_ptr<VisitModel> visit_model,
                     argtype args) : Potential(&args) {
  model_ = std::make_shared<ModelEmpty>();
  visit_model_ = visit_model;
  FEASST_CHECK_ALL_USED(args);
}

Potential::Potential(
    std::shared_ptr<Model> model,
    std::shared_ptr<VisitModel> visit_model,
    argtype args) : Potential(&args) {
  model_ = model;
  visit_model_ = visit_model;
  FEASST_CHECK_ALL_USED(args);
}

Potential::Potential(argtype args) : Potential(&args) {
  DEBUG("parsing model. args: " << str(args));
  model_ = ModelTwoBody().factory(str("Model", &args, "ModelEmpty"), &args);
  DEBUG("parsing visit model. args: " << str(args));
  const std::string name = str("VisitModel", &args, "VisitModel");
  DEBUG("VisitModel name: " << name);
  visit_model_ = VisitModel().factory(name, &args);
  DEBUG(visit_model_->class_name());
  DEBUG("checking args: " << str(args));
  FEASST_CHECK_ALL_USED(args);
}

void Potential::set(const ModelParams& model_params) {
  model_params_override_ = true;
  model_params_ = ModelParams(model_params);
}

void Potential::set_model_param(const std::string& name,
    const int site_type,
    const double value) {
  ASSERT(model_params_override_, "you must first initialize model params "
    << "before setting them.");
  model_params_.set(name, site_type, value);
}
void Potential::set_model_param(const std::string& name,
    const int site_type,
    const double value,
    const Configuration& config) {
  if (!model_params_override_) {
    set_model_params(config);
  }
  set_model_param(name, site_type, value);
}

void Potential::set_model_param(const std::string& name,
    const int site_type0,
    const int site_type1,
    const double value) {
  ASSERT(model_params_override_, "you must first initialize model params "
    << "before setting them.");
  model_params_.set(name, site_type0, site_type1,  value);
}
void Potential::set_model_param(const std::string& name,
    const int site_type0,
    const int site_type1,
    const double value,
    const Configuration& config) {
  if (!model_params_override_) {
    set_model_params(config);
  }
  set_model_param(name, site_type0, site_type1, value);
}


const ModelParams& Potential::model_params() const {
  ASSERT(model_params_override_, "When model parameters are not overridden, "
    << "you must also provide the configuration as an argument.");
  return model_params_;
}

const ModelParams& Potential::model_params(const Configuration& config) const {
  if (model_params_override_) {
    return model_params_;
  }
  return config.model_params();
}

double Potential::energy(Configuration * config) {
  ASSERT(visit_model_, "visitor must be set.");
  if (prevent_cache_ || !cache_.is_unloading(&stored_energy_)) {
    if (model_params_override_) {
      stored_energy_ = model_->compute(model_params_, group_index_, config,
                                       visit_model_.get());
    } else {
      stored_energy_ = model_->compute(group_index_, config, visit_model_.get());
    }
    cache_.load(stored_energy_);
  }
  return stored_energy_;
}

double Potential::select_energy(const Select& select, Configuration * config) {
  ASSERT(visit_model_, "visitor must be set.");
  if (prevent_cache_ || !cache_.is_unloading(&stored_energy_)) {
    if (model_params_override_) {
      stored_energy_ = model_->compute(model_params_, select, group_index_,
                                       config, visit_model_.get());
    } else {
      stored_energy_ = model_->compute(select, group_index_, config,
                                       visit_model_.get());
    }
    cache_.load(stored_energy_);
  }
  return stored_energy_;
}

int Potential::cell_index() const {
  ASSERT(visit_model_->class_name() == "VisitModelCell", "error");
  return group_index();
}

void Potential::precompute(Configuration * config) {
  if (!group_.empty()) {
    group_index_ = config->group_index(group_);
  }
  visit_model_->precompute(config);
  const ModelParams& params = model_params(*config);
  model_->precompute(params);
  ASSERT(config->dimension() == 2 || config->dimension() == 3,
    "Domain must be 2 or 3 dimensions. Please initialize Domain::side_length.");
  const double max_cutoff = maximum(params.select("cutoff").values());
  const double half_min_side = 0.5*config->domain().inscribed_sphere_diameter();
  if (max_cutoff - NEAR_ZERO > half_min_side) {
    WARN("The maximum cutoff:" << max_cutoff << " is greater than half the " <<
         "minimum side length: " << half_min_side);
  }

  if (table_size_ > 0) {
    ASSERT(model_->num_body() == 2, "tables are only implemented for two "
      << "body simulations");
    auto table = MakeModelTwoBodyTable();
    table->precompute(config->model_params());
    table->set(model_params(*config),
      table_size_,
      config->num_site_types(),
      model_);
    model_ = table;
    table_size_ = 0;
  }

  // ModelParam override args
  argtype args = override_args_;
  for (std::map<std::string, std::shared_ptr<ModelParam>>::iterator iter = ModelParam().deserialize_map().begin(); iter != ModelParam().deserialize_map().end(); ++iter) {
    const std::string param = iter->first;
    if (used(param, args)) {
      const double value = dble(param, &args);
      for (int site_type = 0; site_type < config->num_site_types(); ++site_type) {
        set_model_param(param, site_type, value, *config);
      }
    }
    for (int site_type = 0; site_type < config->num_site_types(); ++site_type) {
      std::string param_arg = param + str(site_type);
      if (used(param_arg, args)) {
        set_model_param(param, site_type, dble(param_arg, &args), *config);
      }
    }
    for (int site1 = 0; site1 < config->num_site_types(); ++site1) {
      for (int site2 = site1; site2 < config->num_site_types(); ++site2) {
        std::string param_arg = param + str(site1) + "_" + str(site2);
        if (used(param_arg, args)) {
          set_model_param(param, site1, site2, dble(param_arg, &args), *config);
        }
      }
    }
  }
  FEASST_CHECK_ALL_USED(args);
}

void Potential::check(const Configuration& config) const {
  visit_model_->check(config);
}

void Potential::serialize(std::ostream& ostr) const {
  feasst_serialize_version(432, ostr);
  feasst_serialize(group_index_, ostr);
  feasst_serialize(group_, ostr);
  feasst_serialize_fstdr(visit_model_, ostr);
  feasst_serialize_fstdr(model_, ostr);
  feasst_serialize(stored_energy_, ostr);
  feasst_serialize(model_params_override_, ostr);
  if (model_params_override_) {
    feasst_serialize_fstobj(model_params_, ostr);
  }
  feasst_serialize_fstobj(cache_, ostr);
  feasst_serialize(prevent_cache_, ostr);
  feasst_serialize(table_size_, ostr);
  feasst_serialize(override_args_, ostr);
}

Potential::Potential(std::istream& istr) {
  const int version = feasst_deserialize_version(istr);
  ASSERT(432 == version, "version mismatch: " << version);
  feasst_deserialize(&group_index_, istr);
  feasst_deserialize(&group_, istr);
  // feasst_deserialize_fstdr(visit_model_, istr);
  { // HWH for unknown reasons the above template function does not work
    int existing;
    istr >> existing;
    if (existing != 0) {
      visit_model_ = visit_model_->deserialize(istr);
    }
  }
  // feasst_deserialize_fstdr(model_, istr);
  { // HWH for unknown reasons the above template function does not work
    int existing;
    istr >> existing;
    if (existing != 0) {
      model_ = model_->deserialize(istr);
    }
  }
  feasst_deserialize(&stored_energy_, istr);
  feasst_deserialize(&model_params_override_, istr);
  if (model_params_override_) {
    feasst_deserialize_fstobj(&model_params_, istr);
  }
  feasst_deserialize_fstobj(&cache_, istr);
  feasst_deserialize(&prevent_cache_, istr);
  feasst_deserialize(&table_size_, istr);
  feasst_deserialize(&override_args_, istr);
}

void Potential::set_model_params(const Configuration& config) {
  set(config.model_params().deep_copy());
}

void Potential::synchronize_(const Potential& potential,
    const Select& perturbed) {
  visit_model_->synchronize_(potential.visit_model(), perturbed);
  stored_energy_ = potential.stored_energy_;
}

}  // namespace feasst
