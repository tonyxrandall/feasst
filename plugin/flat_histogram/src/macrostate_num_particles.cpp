
#include <algorithm>
#include "flat_histogram/include/macrostate_num_particles.h"

namespace feasst {

class MapMacrostateNumParticles {
 public:
  MapMacrostateNumParticles() {
    auto hist = MakeHistogram({{"width", "1"}, {"max", "1"}});
    MacrostateNumParticles(*hist).deserialize_map()["MacrostateNumParticles"] =
      MakeMacrostateNumParticles(*hist);
  }
};

static MapMacrostateNumParticles mapper_ = MapMacrostateNumParticles();

std::shared_ptr<Macrostate> MacrostateNumParticles::create(std::istream& istr) const {
  return std::make_shared<MacrostateNumParticles>(istr);
}

MacrostateNumParticles::MacrostateNumParticles(std::istream& istr)
  : Macrostate(istr) {
  feasst_deserialize_version(istr);
}

void MacrostateNumParticles::serialize(std::ostream& ostr) const {
  ostr << class_name_ << " ";
  serialize_macrostate_(ostr);
  feasst_serialize_version(1, ostr);
}

}  // namespace feasst
