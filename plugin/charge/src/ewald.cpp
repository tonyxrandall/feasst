#include <cmath>  // isnan, pow
#include "utils/include/serialize.h"
#include "utils/include/utils.h"  // find_in_list
#include "math/include/constants.h"
#include "configuration/include/domain.h"
#include "configuration/include/visit_configuration.h"
#include "charge/include/ewald.h"

namespace feasst {

Ewald::Ewald(argtype args) : Ewald(&args) { FEASST_CHECK_ALL_USED(args); }
Ewald::Ewald(argtype * args) {
  class_name_ = "Ewald";
  if (used("tolerance", *args)) {
    tolerance_ = std::make_shared<double>(dble("tolerance", args));
  }
  if (used("tolerance_num_sites", *args)) {
    tolerance_num_sites_ = std::make_shared<int>(integer("tolerance_num_sites", args));
  }
  if (used("alpha", *args)) {
    alpha_arg_ = std::make_shared<double>(dble("alpha", args));
  }
  if (used("kxmax", *args)) {
    kxmax_arg_ = std::make_shared<int>(integer("kxmax", args));
  }
  if (used("kymax", *args)) {
    kymax_arg_ = std::make_shared<int>(integer("kymax", args));
  }
  if (used("kzmax", *args)) {
    kzmax_arg_ = std::make_shared<int>(integer("kzmax", args));
  }
  if (used("kmax_squared", *args)) {
    kmax_sq_arg_ = std::make_shared<int>(integer("kmax_squared", args));
  }
  data_.get_dble_1D()->resize(1);
}

void Ewald::tolerance_to_alpha_ks(const double tolerance,
    const Configuration& config, double * alpha,
    int * kxmax, int * kymax, int * kzmax) {
  const double cutoff = config.model_params().select("cutoff").max();
  int num_sites = config.num_sites();
  if (tolerance_num_sites_) {
    num_sites = *tolerance_num_sites_;
  }
  ASSERT(num_sites > 0, "the number of sites: " << num_sites
    << " must be > 0");

  // determine alpha
  *alpha = std::sqrt(num_sites*cutoff*config.domain().volume());
  *alpha *= tolerance/(2.*sum_squared_charge_(config));
  if (*alpha >= 1.) {
    *alpha = (1.35 - 0.15*log(tolerance))/cutoff; // from LAMMPS
  } else {
    *alpha = std::sqrt(-log(*alpha))/cutoff;
  }
  DEBUG("alpha: " << *alpha);
  ASSERT(!std::isnan(*alpha), "alpha is nan");
  *kxmax = estimate_kmax_(*alpha, config, tolerance, 0, num_sites);
  *kymax = estimate_kmax_(*alpha, config, tolerance, 1, num_sites);
  *kzmax = estimate_kmax_(*alpha, config, tolerance, 2, num_sites);
  DEBUG("numkxyz " << *kxmax << " " << *kymax << " " << *kzmax);
}

void Ewald::update_wave_vectors(const Configuration& config) {
  wave_prefactor_.clear();
  wave_num_.clear();
  std::vector<double> kvect(3);
  Position kvec;
  kvec.set_to_origin_3D();
  const Domain& domain = config.domain();
  ASSERT(config.dimension() == 3, "assumes 3D");
  const double lx = domain.side_length(0);
  const double ly = domain.side_length(1);
  const double lz = domain.side_length(2);
  const double xy = domain.xy();
  const double xz = domain.xz();
  const double yz = domain.yz();
  ux_ = 2*PI/lx;
  uy_ = 2*PI*(-xy)/lx/ly;
  uz_ = 2*PI*(xy*yz - ly*xz)/lx/ly/lz;
  vy_ = 2*PI/ly;
  vz_ = 2*PI*(-yz)/ly/lz;
  wz_ = 2*PI/lz;
  const double volume = domain.volume();
  const double alpha = config.model_params().property("alpha");
  DEBUG("kxyzmax " << kxmax_ << " " << kymax_ << " " << kzmax_);
  for (int kx = 0; kx <= kxmax_; ++kx) {
  for (int ky = -kymax_; ky <= kymax_; ++ky) {
  for (int kz = -kzmax_; kz <= kzmax_; ++kz) {
    kvec.set_vector({
      kx*ux_,
      //2.*PI*kx/lx,
      kx*uy_ + ky*vy_,
      //2.*PI*(ky/ly - kx*xy/lx/ly),
      kx*uz_ + ky*vz_ + kz*wz_});
      //2.*PI*(kz/lz - ky*yz/ly/lz + kx*(xy*yz - ly*xz)/lx/ly/lz)});
    const double k_sq = kvec.squared_distance();
    if ( (k_sq < kmax_squared_) && (std::abs(k_sq) > NEAR_ZERO) ) { // allen tildesley, srsw
    //if ( (k_sq <= kmax_squared_) && (std::abs(k_sq) > NEAR_ZERO) ) { // gerhard
      double factor = 1.;
      if (kx != 0) factor = 2;
      wave_prefactor_.push_back(2.*PI*factor*exp(-k_sq/4./alpha/alpha)/k_sq/volume);
      DEBUG(wave_prefactor_.back() << " k2 " << k_sq << " alpha " << alpha << "  vol " << volume);
      DEBUG(2.*PI*factor*exp(-k_sq/4./alpha/alpha));
      DEBUG("kxyz " << kx << " " << ky << " " << kz);
      wave_num_.push_back(kx);
      wave_num_.push_back(ky);
      wave_num_.push_back(kz);
    }
  }}}
  DEBUG("num vectors " << num_vectors());
  ASSERT(num_vectors() > 0, "num_vectors: " << num_vectors());
  data_.get_dble_2D()->resize(2);
  struct_fact_real_()->resize(num_vectors());
  struct_fact_imag_()->resize(num_vectors());
  struct_fact_real_new_.resize(num_vectors());
  struct_fact_imag_new_.resize(num_vectors());
}

std::vector<std::string> Ewald::eik_gen_() {
  std::vector<std::string> eiks;
  std::stringstream ss;
  for (std::string comp : {"r", "i"}) {
    for (std::string coord : {"x", "y", "z"}) {
      int num_k = -1;
      if (coord == "x") {
        num_k = num_kx_;
      } else if (coord == "y") {
        num_k = num_ky_;
      } else if (coord == "z") {
        num_k = num_kz_;
      } else {
        ERROR("unrecognized coord: " << coord);
      }
      for (int k = 0; k < num_k; ++k) {
        ss.str("");
        ss << "eik" << comp << coord << k;
        eiks.push_back(ss.str());
      }
    }
  }
  return eiks;
}

void Ewald::precompute(Configuration * config) {
  VisitModel::precompute(config);
  if (kmax_sq_arg_ && alpha_arg_) {
    ASSERT(!kxmax_arg_ && !kymax_arg_ && !kzmax_arg_,
      "kmax_squared argument overrides k[x,y,z]max arguments.");
    ASSERT(config->domain().is_cubic(),
      "Domain must be cubic to set kmax_squared");
    const int kmax_ = static_cast<int>(std::sqrt(*kmax_sq_arg_)) + 1;
    kxmax_ = kmax_;
    kymax_ = kmax_;
    kzmax_ = kmax_;
    kmax_squared_ = *kmax_sq_arg_*std::pow(2.*PI/config->domain().min_side_length(), 2);
    DEBUG("kmax_squared_ " << kmax_squared_);
    config->add_or_set_model_param("alpha", *alpha_arg_);
  } else {
    if (tolerance_) {
      ASSERT(!alpha_arg_ && !kxmax_arg_ && !kymax_arg_ && !kzmax_arg_,
        "tolerance overrides all other arguments");
      double alpha;
      tolerance_to_alpha_ks(*tolerance_, *config, &alpha, &kxmax_, &kymax_, &kzmax_);
      config->add_or_set_model_param("alpha", alpha);
    } else {
      ASSERT(kxmax_arg_ && kymax_arg_ && kzmax_arg_,
        "k[x,y,z]max arguments required if kmax_squared or tolerance not provided");
      kxmax_ = *kxmax_arg_;
      kymax_ = *kymax_arg_;
      kzmax_ = *kzmax_arg_;
      ASSERT(alpha_arg_,
        "if tolerance is not given, then alpha is required");
      config->add_or_set_model_param("alpha", *alpha_arg_);
    }
    double gsqxmx = std::pow(2*PI*kxmax_/config->domain().side_length(0), 2);
    double gsqymx = std::pow(2*PI*kymax_/config->domain().side_length(1), 2);
    double gsqzmx = std::pow(2*PI*kzmax_/config->domain().side_length(2), 2);
    DEBUG("gsqxmx " << gsqxmx);
    DEBUG("2pi/lx " << 2*PI/config->domain().side_length(0));
    kmax_squared_ = std::max(gsqxmx, gsqymx);
    kmax_squared_ = std::max(kmax_squared_, gsqzmx);
    //kmax_squared_ *= 1.00001;
  }
  num_kx_ = kxmax_ + 1;
  num_ky_ = 2*kymax_ + 1;
  num_kz_ = 2*kzmax_ + 1;
  update_wave_vectors(*config);
  INFO("alpha: " << config->model_params().property("alpha"));
  INFO("kmax_squared " << kmax_squared_);
}

void Ewald::resize_eik_(const Configuration& config) {
  int num_p = config.particles().num();
  int extra = num_p - eik().size();
  if (extra > 0) {
    eik_()->resize(eik_()->size() + extra);
    for (int lastp = num_p - extra; lastp < num_p; ++lastp) {
      const int num_sites = config.particles().particle(lastp).num_sites();
      (*eik_())[lastp].resize(num_sites);
      for (int site = 0; site < num_sites; ++site) {
        (*eik_())[lastp][site].resize(2*(num_kx_ + num_ky_ + num_kz_));
      }
    }
  }
}

void Ewald::resize_eik_(
  const std::vector<std::vector<std::vector<double> > >& eik2) {
  int num_p = static_cast<int>(eik2.size());
  int extra = num_p - eik().size();
  if (extra > 0) {
    eik_()->resize(eik_()->size() + extra);
    for (int lastp = num_p - extra; lastp < num_p; ++lastp) {
      const int num_sites = static_cast<int>(eik2[lastp].size());
      (*eik_())[lastp].resize(num_sites);
      for (int site = 0; site < num_sites; ++site) {
        (*eik_())[lastp][site].resize(2*(num_kx_ + num_ky_ + num_kz_));
      }
    }
  }
}

void Ewald::update_struct_fact_eik(const Select& selection,
    const Configuration&  config,
    std::vector<double> * sf_real,
    std::vector<double> * sf_imag,
    std::vector<std::vector<std::vector<double> > > * eik_new) const {
  ASSERT(charge_index() != -1, "error");
  DEBUG("select " << selection.str());
  ASSERT(sf_real->size() == struct_fact_real().size(),
    "While struct_fact_real_ is of size: " << struct_fact_real().size() <<
    " struct_fact_real is of size: " << sf_real->size());
  std::stringstream ss;
//  const Domain& domain = config.domain();
//  const double lx = domain.side_length(0);
//  const double ly = domain.side_length(1);
//  const double lz = domain.side_length(2);
//  const double xy = domain.xy();
//  const double xz = domain.xz();
//  const double yz = domain.yz();
//  const double twopilx = 2.*PI/lx,
//               twopily = 2.*PI/ly,
//               twopilz = 2.*PI/lz;
  const int state = selection.trial_state();

  // resize eik_new
  {
    int num_p = selection.num_particles();
    int extra = num_p - eik_new->size();
    if (extra > 0) {
      eik_new->resize(eik_new->size() + extra);
      for (int lastp = num_p - extra; lastp < num_p; ++lastp) {
        const int num_sites = selection.num_sites(lastp);
        (*eik_new)[lastp].resize(num_sites);
        for (int site = 0; site < num_sites; ++site) {
          (*eik_new)[lastp][site].resize(2*(num_kx_ + num_ky_ + num_kz_));
        }
      }
    }

    // check size of sites in each particle
    for (int p = 0; p < num_p; ++p) {
      int num_sites = selection.num_sites(p);
      int extra_s = num_sites - static_cast<int>((*eik_new)[p].size());
      if (extra_s > 0) {
        (*eik_new)[p].resize(num_sites);
        for (int lasts = num_sites - extra_s; lasts < num_sites; ++lasts) {
          (*eik_new)[p][lasts].resize(2*(num_kx_ + num_ky_ + num_kz_));
        }
      }
    }
  }

  for (int select_index = 0;
       select_index < selection.num_particles();
       ++select_index) {
    const int part_index = selection.particle_index(select_index);
    const double struct_sign = sign_(selection, select_index);
    for (int ss_index = 0; ss_index < selection.num_sites(select_index); ++ss_index) {
      const int site_index = selection.site_index(select_index, ss_index);
      const Site& site = config.select_particle(part_index).site(site_index);
      if (site.is_physical()) {
        const int eikrx0_index = 0.;
        const int eikry0_index = eikrx0_index + kxmax_ + kymax_ + 1;//num_kx_ + kmax_;
        const int eikrz0_index = eikry0_index + kymax_ + kzmax_ + 1;//num_ky_;
        const int eikix0_index = eikrz0_index + kzmax_ + 1;//num_kx_ + num_ky_ + num_kz_;
        const int eikiy0_index = eikix0_index + kxmax_ + kymax_ + 1;//num_kx_ + kmax_;
        const int eikiz0_index = eikiy0_index + kymax_ + kzmax_ + 1;//num_ky_;
        TRACE(eikrx0_index << " " << eikry0_index << " " << eikrz0_index << " "
          << eikix0_index << " " << eikiy0_index << " " << eikiz0_index);
        const std::vector<double> * const_eikn;

        // update the eik of the selection
        if (state == 0 || state == 2) {
          const_eikn = const_cast<const std::vector<double>*>(
            &(eik()[part_index][site_index]));
        } else {
          std::vector<double> * eikn = &(*eik_new)[select_index][ss_index];
          const_eikn = const_cast<std::vector<double>*>(eikn);
          (*eikn)[eikrx0_index] = 1.;
          (*eikn)[eikix0_index] = 0.;
          (*eikn)[eikry0_index] = 1.;
          (*eikn)[eikiy0_index] = 0.;
          (*eikn)[eikrz0_index] = 1.;
          (*eikn)[eikiz0_index] = 0.;

          // calculate eik of kx = +/-1 explicitly
          const std::vector<double>& pos = config.select_particle(part_index).site(site_index).position().coord();
          const double x = pos[0];
          const double y = pos[1];
          const double z = pos[2];
          //const double udotr = 2.*PI*(x/lx - y*xy/lx/ly + z*(xy*yz/lx/ly/lz - xz/lx/lz));
          const double udotr = ux_*x + uy_*y + uz_*z;
          //const double vdotr = 2.*PI*(y/ly - z*yz/ly/lz);
          const double vdotr = vy_*y + vz_*z;
          //const double wdotr = 2.*PI*z/lz;
          const double wdotr = wz_*z;
          (*eikn)[eikrx0_index + 1] = std::cos(udotr);
          (*eikn)[eikix0_index + 1] = std::sin(udotr);
          (*eikn)[eikry0_index + 1] = std::cos(vdotr);
          (*eikn)[eikiy0_index + 1] = std::sin(vdotr);
          (*eikn)[eikrz0_index + 1] = std::cos(wdotr);
          (*eikn)[eikiz0_index + 1] = std::sin(wdotr);
          (*eikn)[eikry0_index - 1] = (*eikn)[eikry0_index + 1];
          (*eikn)[eikiy0_index - 1] = -(*eikn)[eikiy0_index + 1];
          (*eikn)[eikrz0_index - 1] = (*eikn)[eikrz0_index + 1];
          (*eikn)[eikiz0_index - 1] = -(*eikn)[eikiz0_index + 1];

          // compute remaining eik by recursion
          for (int kx = 2; kx <= kxmax_; ++kx) {
            const double eikr2 = (*eikn)[eikrx0_index + kx - 1]*(*eikn)[eikrx0_index + 1] -
              (*eikn)[eikix0_index + kx - 1]*(*eikn)[eikix0_index + 1];
            (*eikn)[eikrx0_index + kx] = eikr2;
            const double eiki2 = (*eikn)[eikrx0_index + kx - 1]*(*eikn)[eikix0_index + 1] +
              (*eikn)[eikix0_index + kx - 1]*(*eikn)[eikrx0_index + 1];
            (*eikn)[eikix0_index + kx] = eiki2;
          }
          for (int ky = 2; ky <= kymax_; ++ky) {
            const double eikr2 = (*eikn)[eikry0_index + ky - 1]*(*eikn)[eikry0_index + 1] -
              (*eikn)[eikiy0_index + ky - 1]*(*eikn)[eikiy0_index + 1];
            (*eikn)[eikry0_index + ky] = eikr2;
            const double eiki2 = (*eikn)[eikry0_index + ky - 1]*(*eikn)[eikiy0_index + 1] +
              (*eikn)[eikiy0_index + ky - 1]*(*eikn)[eikry0_index + 1];
            (*eikn)[eikiy0_index + ky] = eiki2;
            (*eikn)[eikry0_index - ky] = eikr2;
            (*eikn)[eikiy0_index - ky] = -eiki2;
          }
          for (int kz = 2; kz <= kzmax_; ++kz) {
            const double eikr2 = (*eikn)[eikrz0_index + kz - 1]*(*eikn)[eikrz0_index + 1] -
              (*eikn)[eikiz0_index + kz - 1]*(*eikn)[eikiz0_index + 1];
            (*eikn)[eikrz0_index + kz] = eikr2;
            const double eiki2 = (*eikn)[eikrz0_index + kz - 1]*(*eikn)[eikiz0_index + 1] +
              (*eikn)[eikiz0_index + kz - 1]*(*eikn)[eikrz0_index + 1];
            (*eikn)[eikiz0_index + kz] = eiki2;
            (*eikn)[eikrz0_index - kz] = eikr2;
            (*eikn)[eikiz0_index - kz] = -eiki2;
          }
        }

        // compute structure factor
        const int type = site.type();
        const double charge = config.model_params().select(charge_index()).value(type);
        for (int k_index = 0; k_index < num_vectors(); ++k_index) {
          const int kdim = dimension_*k_index;
          const double kx = wave_num_[kdim];
          const double ky = wave_num_[kdim + 1];
          const double kz = wave_num_[kdim + 2];
          TRACE("k " << k_index << " kx " << kx << " ky " << ky << " kz " << kz << " size " << num_vectors() << " kdim " << kdim);
          const double eikrx = (*const_eikn)[eikrx0_index + kx];
          const double eikix = (*const_eikn)[eikix0_index + kx];
          const double eikry = (*const_eikn)[eikry0_index + ky];
          const double eikiy = (*const_eikn)[eikiy0_index + ky];
          const double eikrz = (*const_eikn)[eikrz0_index + kz];
          const double eikiz = (*const_eikn)[eikiz0_index + kz];
          TRACE("eik[r,i]x " << eikrx << " " << eikix << " y " << eikry << " " << eikiy << " z " << eikrz << " " << eikiz << " sz " << const_eikn->size());
          const double eikr = eikrx*eikry*eikrz
                     - eikix*eikiy*eikrz
                     - eikix*eikry*eikiz
                     - eikrx*eikiy*eikiz;
          const double eiki = -eikix*eikiy*eikiz
                     + eikrx*eikry*eikiz
                     + eikrx*eikiy*eikrz
                     + eikix*eikry*eikrz;
          TRACE("charge " << charge << " eikr " << eikr << " eiki " << eiki << " sign " << struct_sign);
          TRACE(sf_real->size());
          (*sf_real)[k_index] += struct_sign*charge*eikr;
          (*sf_imag)[k_index] += struct_sign*charge*eiki;
        }
      }
    }
  }
}

class MapEwald {
 public:
  MapEwald() {
    Ewald().deserialize_map()["Ewald"] = std::make_shared<Ewald>();
  }
};

static MapEwald mapper_ = MapEwald();

void Ewald::serialize(std::ostream& ostr) const {
  ostr << class_name_ << " ";
  serialize_visit_model_(ostr);
  feasst_serialize_version(319, ostr);
  feasst_serialize_sp(tolerance_, ostr);
  feasst_serialize_sp(tolerance_num_sites_, ostr);
  feasst_serialize_sp(alpha_arg_, ostr);
  feasst_serialize_sp(kxmax_arg_, ostr);
  feasst_serialize_sp(kymax_arg_, ostr);
  feasst_serialize_sp(kzmax_arg_, ostr);
  feasst_serialize_sp(kmax_sq_arg_, ostr);
  feasst_serialize(kxmax_, ostr);
  feasst_serialize(kymax_, ostr);
  feasst_serialize(kzmax_, ostr);
  feasst_serialize(kmax_squared_, ostr);
  feasst_serialize(num_kx_, ostr);
  feasst_serialize(num_ky_, ostr);
  feasst_serialize(num_kz_, ostr);
  feasst_serialize(wave_prefactor_, ostr);
  feasst_serialize(wave_num_, ostr);
  feasst_serialize(ux_, ostr);
  feasst_serialize(uy_, ostr);
  feasst_serialize(uz_, ostr);
  feasst_serialize(vy_, ostr);
  feasst_serialize(vz_, ostr);
  feasst_serialize(wz_, ostr);
  feasst_serialize(struct_fact_real_new_, ostr);
  feasst_serialize(struct_fact_imag_new_, ostr);
  DEBUG("size: " << ostr.tellp());
}

Ewald::Ewald(std::istream& istr) : VisitModel(istr) {
  const int version = feasst_deserialize_version(istr);
  ASSERT(319 == version, version);
//  feasst_deserialize(tolerance_, istr);
//  feasst_deserialize(alpha_arg_, istr);
  double value;
  int existing;
  istr >> existing;
  if (existing != 0) {
    istr >> value;
    tolerance_ = std::make_shared<double>(value);
  }
  istr >> existing;
  if (existing != 0) {
    istr >> existing;
    tolerance_num_sites_ = std::make_shared<int>(existing);
  }
  istr >> existing;
  if (existing != 0) {
    istr >> value;
    alpha_arg_ = std::make_shared<double>(value);
  }
  //feasst_deserialize(kxmax_arg_, istr);
  //feasst_deserialize(kymax_arg_, istr);
  //feasst_deserialize(kzmax_arg_, istr);
  //feasst_deserialize(kmax_sq_arg_, istr);
  int int_value;
  istr >> existing;
  if (existing != 0) {
    istr >> int_value;
    kxmax_arg_ = std::make_shared<int>(int_value);
  }
  istr >> existing;
  if (existing != 0) {
    istr >> int_value;
    kymax_arg_ = std::make_shared<int>(int_value);
  }
  istr >> existing;
  if (existing != 0) {
    istr >> int_value;
    kzmax_arg_ = std::make_shared<int>(int_value);
  }
  istr >> existing;
  if (existing != 0) {
    istr >> int_value;
    kmax_sq_arg_ = std::make_shared<int>(int_value);
  }
  feasst_deserialize(&kxmax_, istr);
  feasst_deserialize(&kymax_, istr);
  feasst_deserialize(&kzmax_, istr);
  feasst_deserialize(&kmax_squared_, istr);
  feasst_deserialize(&num_kx_, istr);
  feasst_deserialize(&num_ky_, istr);
  feasst_deserialize(&num_kz_, istr);
  feasst_deserialize(&wave_prefactor_, istr);
  feasst_deserialize(&wave_num_, istr);
  feasst_deserialize(&ux_, istr);
  feasst_deserialize(&uy_, istr);
  feasst_deserialize(&uz_, istr);
  feasst_deserialize(&vy_, istr);
  feasst_deserialize(&vz_, istr);
  feasst_deserialize(&wz_, istr);
  feasst_deserialize(&struct_fact_real_new_, istr);
  feasst_deserialize(&struct_fact_imag_new_, istr);
}

class SumCharge : public LoopConfigOneBody {
 public:
  SumCharge(int charge_index) : charge_index_(charge_index) {}
  void work(const Site& site,
      const Configuration& config,
      const LoopDescriptor& data) override {
    const int type = site.type();
    ASSERT(charge_index_ != -1, "error");
    charge_ += config.model_params().select(charge_index_).value(type);
  }
  double charge() const { return charge_; }
 private:
  double charge_ = 0.;
  int charge_index_;
};

double Ewald::net_charge(const Configuration& config) const {
  SumCharge sum(config.model_params().index("charge"));
  VisitConfiguration().loop(config, &sum);
  return sum.charge();
}

void Ewald::compute(
    ModelOneBody * model,
    const ModelParams& model_params,
    Configuration * config,
    const int group_index) {
  std::fill(struct_fact_real_new_.begin(), struct_fact_real_new_.end(), 0.);
  std::fill(struct_fact_imag_new_.begin(), struct_fact_imag_new_.end(), 0.);
  resize_eik_(*config);
  update_struct_fact_eik(config->group_select(group_index), *config,
                         &struct_fact_real_new_,
                         &struct_fact_imag_new_,
                         &eik_new_);
  const double conversion = model_params.constants().charge_conversion();
  stored_energy_new_ = conversion*fourier_energy_(struct_fact_real_new_,
                                                  struct_fact_imag_new_);
  DEBUG("stored_energy_ " << stored_energy_new_);
  set_energy(stored_energy_new_);
  finalizable_ = true;
}

void Ewald::compute(
    ModelOneBody * model,
    const ModelParams& model_params,
    const Select& selection,
    Configuration * config,
    const int group_index) {
  ASSERT(group_index == 0, "group index cannot be varied because redundant." <<
    "otherwise implement filtering of selection based on group.");
  double enrg = 0.;
  DEBUG("selection.trial_state() " << selection.trial_state());
  const int state = selection.trial_state();
  DEBUG("state " << state);
  ASSERT(state == 0 ||
         state == 1 ||
         state == 2 ||
         state == 3,
    "unrecognized trial_state: " << state);

  // initialize new structure factor, unless its a new move position
  if (state != 1) {
    struct_fact_real_new_ = struct_fact_real();
    DEBUG("size " << struct_fact_real_new_.size() << " " <<
          struct_fact_real().size());
    struct_fact_imag_new_ = struct_fact_imag();
  }
  resize_eik_(*config);
  update_struct_fact_eik(selection, *config, &struct_fact_real_new_,
                                             &struct_fact_imag_new_,
                                             &eik_new_);
  // compute new energy
  if (state != 0) {
    const double conversion = model_params.constants().charge_conversion();
    stored_energy_new_ = conversion*fourier_energy_(struct_fact_real_new_,
                                                    struct_fact_imag_new_);
  }
  if (state == 0) {
    enrg = stored_energy();
  } else if (state == 1) {
    enrg = stored_energy_new_;
  } else if (state == 2) {
    // contribution = energy with - energy without
    // for remove, energy old - energy new
    enrg = stored_energy() - stored_energy_new_;
  } else if (state == 3) {
    // contribution = energy with - energy without
    // for add, energy new - energy old
    enrg = stored_energy_new_ - stored_energy();
  }
  DEBUG("enrg: " << enrg);
  DEBUG("stored_energy_ " << stored_energy() << " "
       "stored_energy_new_ " << stored_energy_new_);
  set_energy(enrg);
  finalizable_ = true;
}

void Ewald::finalize(const Select& select, Configuration * config) {
  VisitModel::finalize(select, config);
  if (finalizable_) {
    DEBUG("finalizing");
    ASSERT(struct_fact_real_new_.size() > 0, "error");
    *stored_energy_() = stored_energy_new_;
    *struct_fact_real_() = struct_fact_real_new_;
    *struct_fact_imag_() = struct_fact_imag_new_;
    finalizable_ = false;
    DEBUG("select " << select.str());

    resize_eik_(*config);

    // update eik using eik_new
    DEBUG(select.trial_state());
    if (select.trial_state() != 2) {
      for (int ipart = 0; ipart < select.num_particles(); ++ipart) {
        const int part_index = select.particle_index(ipart);
        for (int isite = 0; isite < select.num_sites(ipart); ++isite) {
          const int site_index = select.site_index(ipart, isite);
          const std::vector<double>& eik_new = eik_new_[ipart][isite];
          for (int k = 0; k < static_cast<int>(eik_new.size()); ++k) {
            //INFO("part_index " << part_index << " sz " << (*eik_()).size());
            (*eik_())[part_index][site_index][k] = eik_new[k];
          }
        }
      }
    }
  }
}

void Ewald::check_size() const {
  ASSERT(wave_prefactor_.size() == wave_num_.size(), "size err");
}

double Ewald::fourier_rms_(
    const double alpha,
    const int kmax,
    const Configuration& config,
    const int dimen,
    const int num_sites) {
  ASSERT(num_sites > 0, "error");
  ASSERT(!std::isnan(alpha), "alpha is nan");
  DEBUG("alpha: " << alpha);
  const double side_length = config.domain().side_length(dimen);
  return 2.*sum_squared_charge_(config)*alpha/side_length *
    std::sqrt(1./(PI*kmax*num_sites)) *
    exp(-std::pow(PI*kmax/alpha/side_length, 2));
}

int Ewald::estimate_kmax_(
    const double alpha,
    const Configuration& config,
    const double tolerance,
    const int dimen,
    const int num_sites) {
  int kmax = 0;
  double err = NEAR_INFINITY;
  while (err > tolerance) {
    kmax += 1;
    err = fourier_rms_(alpha, kmax, config, dimen, num_sites);
  }
  return kmax;
}

double Ewald::fourier_energy_(const std::vector<double>& struct_fact_real,
                              const std::vector<double>& struct_fact_imag) {
  double en = 0;
  for (int k = 0; k < num_vectors(); ++k) {
    en += wave_prefactor_[k]*(struct_fact_real[k]*struct_fact_real[k]
                            + struct_fact_imag[k]*struct_fact_imag[k]);
  }
  return en;
}

double Ewald::sign_(const Select& select, const int pindex) const {
  int state = select.trial_state();
  DEBUG("state " << state);
  // ASSERT(state != -1, "error, state: " << state);
  if (state == 0 || state == 2) {
    return -1.0;
  }
  return 1.0;
}

std::vector<double> * Ewald::struct_fact_real_() {
  return &((*data_.get_dble_2D())[0]);
}

std::vector<double> * Ewald::struct_fact_imag_() {
  return &((*data_.get_dble_2D())[1]);
}

std::vector<std::vector<std::vector<double> > > * Ewald::eik_() {
  return &(*manual_data_.get_dble_3D());
}

void Ewald::change_volume(const double delta_volume, const int dimension) {
  FATAL("not implemented");
}

void Ewald::synchronize_(const VisitModel& visit, const Select& select) {
  VisitModel::synchronize_(visit, select);
  DEBUG("select " << select.str());
  resize_eik_(visit.manual_data().dble_3D());
  for (int ipart = 0; ipart < select.num_particles(); ++ipart) {
    const int part_index = select.particle_index(ipart);
    for (int isite = 0; isite < select.num_sites(ipart); ++isite) {
      const int site_index = select.site_index(ipart, isite);
      const std::vector<double>& eik_new =
        visit.manual_data().dble_3D()[part_index][site_index];
      for (int k = 0; k < static_cast<int>(eik_new.size()); ++k) {
        //INFO(part_index << " " << site_index << " " << k << " " << eik_new[k]);
        ASSERT(part_index < static_cast<int>((*eik_()).size()),
          "part_index: " << part_index << " >= size: " << (*eik_()).size());
        (*eik_())[part_index][site_index][k] = eik_new[k];
      }
    }
  }
}

void Ewald::check(const Configuration& config) const {
  // check the eiks
  std::vector<double> sf_real(struct_fact_real().size());
  std::vector<double> sf_imag(struct_fact_real().size());
  std::vector<std::vector<std::vector<double> > > eikn;
  const Select& sel = config.selection_of_all();
  update_struct_fact_eik(sel, config, &sf_real, &sf_imag, &eikn);
  DEBUG(config.selection_of_all().str());
  DEBUG(eikn.size());
  const double tolerance = 1e-8;
  ASSERT(is_equal(sf_real, struct_fact_real(), tolerance),
    "sf_real: " << feasst_str(sf_real)
    << "struct_fact_real() << " << feasst_str(struct_fact_real()));
  ASSERT(is_equal(sf_imag, struct_fact_imag(), tolerance),
    "sf_imag: " << feasst_str(sf_imag)
    << "struct_fact_imag() << " << feasst_str(struct_fact_imag()));
  for (int sp = 0; sp < sel.num_particles(); ++sp) {
    const int part = sel.particle_index(sp);
    ASSERT(is_equal(eikn[sp], eik()[part], tolerance), "eikn " << feasst_str(eikn[sp])
      << " eik " << feasst_str(eik()[part]));
  }
}

int Ewald::wave_num(const int vector_index, const int dim) const {
  return wave_num_[dimension_*vector_index + dim];
}

double Ewald::eik(const int part_index, const int site_index,
    const int vector_index, const int dim, const bool real) const {
  // copy-pasted from update_struc_fact
  const int eikrx0_index = 0.;
  const int eikry0_index = eikrx0_index + kxmax_ + kymax_ + 1;//num_kx_ + kmax_;
  const int eikrz0_index = eikry0_index + kymax_ + kzmax_ + 1;//num_ky_;
  const int eikix0_index = eikrz0_index + kzmax_ + 1;//num_kx_ + num_ky_ + num_kz_;
  const int eikiy0_index = eikix0_index + kxmax_ + kymax_ + 1;//num_kx_ + kmax_;
  const int eikiz0_index = eikiy0_index + kymax_ + kzmax_ + 1;//num_ky_;
  int index = -1;
  if (dim == 0) {
    if (real) {
      index = eikrx0_index + wave_num(vector_index, dim);
    } else {
      index = eikix0_index + wave_num(vector_index, dim);
    }
  } else if (dim == 1) {
    if (real) {
      index = eikry0_index + wave_num(vector_index, dim);
    } else {
      index = eikiy0_index + wave_num(vector_index, dim);
    }
  } else if (dim == 2) {
    if (real) {
      index = eikrz0_index + wave_num(vector_index, dim);
    } else {
      index = eikiz0_index + wave_num(vector_index, dim);
    }
  }
  return eik()[part_index][site_index][index];
}

}  // namespace feasst
