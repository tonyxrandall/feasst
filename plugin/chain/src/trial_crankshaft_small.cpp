#include "utils/include/debug.h"
#include "utils/include/serialize.h"
#include "chain/include/select_crankshaft_small.h"
#include "chain/include/perturb_crankshaft_small.h"
#include "chain/include/trial_crankshaft_small.h"

namespace feasst {

class MapTrialCrankshaftSmall {
 public:
  MapTrialCrankshaftSmall() {
    auto obj = MakeTrialCrankshaftSmall({{"site", "0"},
                                         {"anchor_site0", "1"},
                                         {"anchor_site1", "2"}});
    obj->deserialize_map()["TrialCrankshaftSmall"] = obj;
  }
};

static MapTrialCrankshaftSmall mapper_ = MapTrialCrankshaftSmall();

TrialCrankshaftSmall::TrialCrankshaftSmall(argtype * args) :
  TrialMove(std::make_shared<SelectCrankshaftSmall>(args),
            std::make_shared<PerturbCrankshaftSmall>(args),
            args) {
  class_name_ = "TrialCrankshaftSmall";
  set_description("TrialCrankshaftSmall");
}
TrialCrankshaftSmall::TrialCrankshaftSmall(argtype args) : TrialCrankshaftSmall(&args) {
  check_all_used(args);
}

TrialCrankshaftSmall::TrialCrankshaftSmall(std::istream& istr) : TrialMove(istr) {
  const int version = feasst_deserialize_version(istr);
  ASSERT(version == 1826, "mismatch version: " << version);
}

void TrialCrankshaftSmall::serialize(std::ostream& ostr) const {
  ostr << class_name_ << " ";
  serialize_trial_move_(ostr);
  feasst_serialize_version(1826, ostr);
}

}  // namespace feasst
