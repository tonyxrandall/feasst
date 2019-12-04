
#ifndef FEASST_SYSTEM_MODEL_TWO_BODY_FACTORY_H_
#define FEASST_SYSTEM_MODEL_TWO_BODY_FACTORY_H_

#include <memory>
#include <string>
#include <vector>
#include "system/include/model_two_body.h"

namespace feasst {

/**
  Contains a collection of two body models.
  This allows the visitor to consider multiple two body models for each
  pairwise interaction.
 */
class ModelTwoBodyFactory : public ModelTwoBody {
 public:
  ModelTwoBodyFactory() {}

  /// Add a two body model.
  void add_model(std::shared_ptr<ModelTwoBody> model) {
    models_.push_back(model); }

  /// Return the number of two body models.
  int num() const { return static_cast<int>(models_.size()); }

  /// Return a two body model by index.
  const Model * model(const int index) const { return models_[index].get(); }

  double energy(
      const double squared_distance,
      const int type1,
      const int type2,
      const ModelParams& model_params) const override;

  void precompute(const ModelParams& existing) override;

  // serialize
  std::shared_ptr<Model> create(std::istream& istr) const override {
    return std::make_shared<ModelTwoBodyFactory>(istr); }
  void serialize(std::ostream& ostr) const override;
  explicit ModelTwoBodyFactory(std::istream& istr);

 private:
  const std::string class_name_ = "ModelTwoBodyFactory";
  std::vector<std::shared_ptr<Model> > models_;
};

inline std::shared_ptr<ModelTwoBodyFactory> MakeModelTwoBodyFactory() {
  return std::make_shared<ModelTwoBodyFactory>();
}

}  // namespace feasst

#endif  // FEASST_SYSTEM_MODEL_TWO_BODY_FACTORY_H_
