#include <vector>
#include "system/include/visit_model.h"
#include "system/include/model_two_body.h"
#include "system/include/model_one_body.h"
#include "system/include/select_list.h"
#include "math/include/constants.h"

namespace feasst {

void VisitModel::compute(
    const ModelOneBody& model,
    const ModelParams& model_params,
    Configuration * config,
    const int group_index) {
  zero_energy();
  const Select& selection = config->group_selects()[group_index];
  for (int select_index = 0;
       select_index < selection.num_particles();
       ++select_index) {
    const int part_index = selection.particle_index(select_index);
    const Particle& part = config->select_particle(part_index);
    for (int site_index : selection.site_indices(select_index)) {
      const Site& site = part.site(site_index);
      if (site.is_physical()) {
        energy_ += model.energy(site, config, model_params);
      }
    }
  }
}

void VisitModel::compute(
    const ModelOneBody& model,
    const ModelParams& model_params,
    const Select& selection,
    Configuration * config,
    const int group_index) {
  ASSERT(group_index == 0, "not implemented because redundant to selection");
  zero_energy();
  DEBUG("HWH: add wrapping of site positions");
  for (int select_index = 0; select_index < selection.num_particles(); ++select_index) {
    const int particle_index = selection.particle_index(select_index);
    const Particle& part = config->select_particle(particle_index);
    for (int site_index : selection.site_indices(select_index)) {
      const Site& site = part.site(site_index);
      if (site.is_physical()) {
        energy_ += model.energy(site, config, model_params);
      }
    }
  }
}

void VisitModel::compute(
    const ModelTwoBody& model,
    const ModelParams& model_params,
    Configuration * config,
    const int group_index) {
  TRACE("VisitModel for TwoBody entire config");
  zero_energy();
  const Domain& domain = config->domain();
  init_relative_(domain, &relative_, &pbc_);
  const Select& selection = config->group_selects()[group_index];
  for (int select1_index = 0;
       select1_index < selection.num_particles() - 1;
       ++select1_index) {
    const int part1_index = selection.particle_index(select1_index);
    for (int select2_index = select1_index + 1;
         select2_index < selection.num_particles();
         ++select2_index) {
      const int part2_index = selection.particle_index(select2_index);
      for (int site1_index : selection.site_indices(select1_index)) {
        for (int site2_index : selection.site_indices(select2_index)) {
          get_inner_()->compute(part1_index, site1_index, part2_index, site2_index,
                                config, model_params, model, false, &relative_,
                                &pbc_);
        }
      }
    }
  }
  set_energy(inner()->energy());
}

void VisitModel::compute(
    const ModelTwoBody& model,
    const ModelParams& model_params,
    const Select& selection,
    Configuration * config,
    const int group_index) {
  DEBUG("visiting model");
  zero_energy();
  const Domain& domain = config->domain();
  init_relative_(domain, &relative_, &pbc_);
  const Select& select_all = config->group_selects()[group_index];
  prep_for_revert(selection);
  bool is_old_config = false;
  if (selection.trial_state() == 0 ||
      selection.trial_state() == 2) {
    is_old_config = true;
  }
  // If only one particle in selection, simply exclude part1==part2
  if (selection.num_particles() == 1) {
    for (int select1_index = 0;
         select1_index < selection.num_particles();
         ++select1_index) {
      const int part1_index = selection.particle_index(select1_index);
      TRACE("part1_index " << part1_index << " s " <<
            selection.particle_indices().size() << " " <<
            selection.site_indices().size());
      for (int select2_index = 0;
           select2_index < select_all.num_particles();
           ++select2_index) {
        const int part2_index = select_all.particle_index(select2_index);
        if (part1_index != part2_index) {
          for (int site1_index : selection.site_indices(select1_index)) {
            TRACE("site1_index " << site1_index);
            for (int site2_index : select_all.site_indices(select2_index)) {
              TRACE("index: " << part1_index << " " << part2_index << " " <<
                    site1_index << " " << site2_index);
              get_inner_()->compute(part1_index, site1_index,
                                    part2_index, site2_index,
                                    config, model_params, model,
                                    is_old_config,
                                    &relative_, &pbc_);
            }
          }
        }
      }
    }
  // If selection is more than one particle, remove those in selection from
  // select_all.
  // Calculate energy in two separate loops.
  } else {
    Select select_others = select_all;
    select_others.remove(selection);
    for (int select1_index = 0;
         select1_index < selection.num_particles();
         ++select1_index) {
      const int part1_index = selection.particle_index(select1_index);
      TRACE("part1_index " << part1_index << " s " <<
            selection.particle_indices().size() << " " <<
            selection.site_indices().size());
      for (int select2_index = 0;
           select2_index < select_others.num_particles();
           ++select2_index) {
        const int part2_index = select_others.particle_index(select2_index);
        for (int site1_index : selection.site_indices(select1_index)) {
          TRACE("site1_index " << site1_index);
          for (int site2_index : select_others.site_indices(select2_index)) {
            TRACE("index: " << part1_index << " " << part2_index << " " <<
                  site1_index << " " << site2_index);
            get_inner_()->compute(part1_index, site1_index,
                                  part2_index, site2_index,
                                  config, model_params, model,
                                  is_old_config,
                                  &relative_, &pbc_);
          }
        }
      }
    }
    for (int select1_index = 0;
         select1_index < selection.num_particles() - 1;
         ++select1_index) {
      const int part1_index = selection.particle_index(select1_index);
      TRACE("sel1 " << select1_index << " part1_index " << part1_index << " s " <<
            selection.particle_indices().size() << " " <<
            selection.site_indices().size());
      for (int select2_index = select1_index + 1;
           select2_index < selection.num_particles();
           ++select2_index) {
        const int part2_index = selection.particle_index(select2_index);
        TRACE("sel2 " << select2_index << " part2_index " << part2_index);
        for (int site1_index : selection.site_indices(select1_index)) {
          TRACE("site1_index " << site1_index);
          for (int site2_index : selection.site_indices(select2_index)) {
            TRACE("index: " << part1_index << " " << part2_index << " " <<
                  site1_index << " " << site2_index);
            get_inner_()->compute(part1_index, site1_index,
                                  part2_index, site2_index,
                                  config, model_params, model,
                                  is_old_config,
                                  &relative_, &pbc_);
          }
        }
      }
    }
  }
  set_energy(inner()->energy());
}

void VisitModel::check_energy(
    const Model& model,
    Configuration * config,
    const int group_index) {
  TRACE("checking energy");
  model.compute(group_index, config, this);
  const double en_group = energy();

  // select each particle and compare half the sum with the whole
  SelectList select;
  double en_select = 0;
  const int num = config->num_particles(group_index);
  for (int part = 0; part < num; ++part) {
    select.particle(part, *config, group_index);
    model.compute(select, group_index, config, this);
    TRACE("part " << part << " en " << energy());
    en_select += 0.5*energy();
  }
  ASSERT(std::abs(en_group - en_select) < num*num*1e-15, "Error with " <<
    "visitor implementation. The energy of " <<
    MAX_PRECISION << "group(" << group_index << "): " << en_group << " "
    "is not consistent with half the sum of the energies of the selected " <<
    "particles: " << en_select << ". The difference is: " <<
    en_group - en_select << " with tolerance: " << num*num*1e-15);
}

class MapVisitModel {
 public:
  MapVisitModel() {
    VisitModel().deserialize_map()["VisitModel"] =
      std::make_shared<VisitModel>();
  }
};

static MapVisitModel mapper_visit_model_ = MapVisitModel();

std::map<std::string, std::shared_ptr<VisitModel> >& VisitModel::deserialize_map() {
  static std::map<std::string, std::shared_ptr<VisitModel> >* ans =
     new std::map<std::string, std::shared_ptr<VisitModel> >();
  return *ans;
}

void VisitModel::serialize_visit_model_(std::ostream& ostr) const {
  feasst_serialize_version(545, ostr);
  feasst_serialize(energy_, ostr);
  feasst_serialize_fstdr(inner_, ostr);
}

//void VisitModel::deserialize_visit_model_(std::istream& istr, std::shared_ptr<VisitModel> visitor) const {
VisitModel::VisitModel(std::istream& istr) {
  const int version = feasst_deserialize_version(istr);
  ASSERT(545 == version, "mismatch: " << version);
  feasst_deserialize(&energy_, istr);
  // feasst_deserialize_fstdr(inner_, istr);
  { // for unknown reason, template function above does not work
    int existing;
    istr >> existing;
    if (existing != 0) {
      inner_ = inner_->deserialize(istr);
    }
  }
}

}  // namespace feasst
