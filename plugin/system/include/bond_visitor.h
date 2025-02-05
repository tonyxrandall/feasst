
#ifndef FEASST_CONFIGURATION_BOND_VISITOR_H_
#define FEASST_CONFIGURATION_BOND_VISITOR_H_

#include <memory>
#include <string>
#include <map>
#include "configuration/include/bond.h"
#include "system/include/bond_two_body.h"
#include "system/include/bond_three_body.h"
#include "system/include/bond_four_body.h"
#include "system/include/rigid_bond.h"
#include "system/include/rigid_angle.h"
#include "system/include/rigid_dihedral.h"

namespace feasst {

class Configuration;
class Select;

class BondVisitor {
 public:
  /**
   args:
   - verbose: print non-zero energies (default: false).
   */
  explicit BondVisitor(argtype args = argtype());
  void compute_all(const Configuration& config, const int group_index = 0);
  void compute_all(const Select& selection, const Configuration& config);
  void compute_two(const Configuration& config, const int group_index = 0);
  void compute_two(const Select& selection, const Configuration& config);
  void compute_three(const Configuration& config, const int group_index = 0);
  void compute_three(const Select& selection, const Configuration& config);
  void compute_four(const Configuration& config, const int group_index = 0);
  void compute_four(const Select& selection, const Configuration& config);
  double energy() const { return energy_; }

  // serialize
  std::string class_name() const { return class_name_; }
  virtual void serialize(std::ostream& ostr) const;
  virtual std::shared_ptr<BondVisitor> create(std::istream& istr) const;
  std::map<std::string, std::shared_ptr<BondVisitor> >& deserialize_map();
  std::shared_ptr<BondVisitor> deserialize(std::istream& istr);
  explicit BondVisitor(std::istream& istr);
  virtual ~BondVisitor() {}

 protected:
  std::string class_name_ = "BondVisitor";

  void serialize_bond_visitor_(std::ostream& ostr) const;

 private:
  double energy_ = 0.;
  double energy_two_body_ = 0.;
  double energy_three_body_ = 0.;
  double energy_four_body_ = 0.;
  bool verbose_;

  // temporary
  RigidBond bond_;
  RigidAngle angle_;
  RigidDihedral dihedral_;
};

inline std::shared_ptr<BondVisitor> MakeBondVisitor(
    const argtype &args = argtype()) {
  return std::make_shared<BondVisitor>(args);
}

}  // namespace feasst

#endif  // FEASST_CONFIGURATION_BOND_VISITOR_H_
