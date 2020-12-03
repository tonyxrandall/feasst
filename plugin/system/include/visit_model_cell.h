
#ifndef FEASST_SYSTEM_VISIT_MODEL_CELL_H_
#define FEASST_SYSTEM_VISIT_MODEL_CELL_H_

#include <memory>
#include "system/include/visit_model.h"

namespace feasst {

/**
  Compute many-body inter-particle interactions using a cell list.
  The cell index is determined by the number of times Configuration::init_cells
  is called.
 */
class VisitModelCell : public VisitModel {
 public:
  VisitModelCell() : VisitModel() {
    class_name_ = "VisitModelCell"; }
  void compute(
      ModelTwoBody * model,
      const ModelParams& model_params,
      Configuration * config,
      const int cell_index = 0) override;
  void compute(
      ModelTwoBody * model,
      const ModelParams& model_params,
      const Select& selection,
      Configuration * config,
      const int cell_index = 0) override;
  std::shared_ptr<VisitModel> create(std::istream& istr) const override;
  void serialize(std::ostream& ostr) const override;
  explicit VisitModelCell(std::istream& istr);
  virtual ~VisitModelCell() {}
};

inline std::shared_ptr<VisitModelCell> MakeVisitModelCell() {
  return std::make_shared<VisitModelCell>();
}

}  // namespace feasst

#endif  // FEASST_SYSTEM_VISIT_MODEL_CELL_H_
