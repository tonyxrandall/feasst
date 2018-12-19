/* This is an interface file for swig.
   This file is created by dev/tools/depend.py . Modifications to this
   file will likely be overwritten in the future. Thus, edit depend.py
   instead.

   usage: /path/to/feasst/py/run.sh /path/to/feasst/dev/tools/depend.py -s /path/to/feasst
 */

%module feasst

%{
#include "core/include/file.h"
#include "core/include/accumulator.h"
#include "core/include/properties.h"
#include "core/include/typed_entity.h"
#include "core/include/utils_math.h"
#include "core/include/position.h"
#include "core/include/visit_bond.h"
#include "core/include/formula.h"
#include "core/include/formula_exponential.h"
#include "core/include/histogram.h"
#include "core/include/formula_polynomial.h"
#include "core/include/bias.h"
#include "core/include/bias_wang_landau.h"
#include "core/include/bias_transition_matrix.h"
#include "core/include/custom_exception.h"
#include "core/include/cells.h"
#include "core/include/utils.h"
#include "core/include/debug.h"
#include "core/include/random.h"
#include "core/include/domain.h"
#include "core/include/domain_triclinic.h"
#include "core/include/domain_cuboid.h"
#include "core/include/physical_constants.h"
#include "core/include/constants.h"
#include "core/include/bond.h"
#include "core/include/utils_io.h"
#include "core/include/site.h"
#include "core/include/particle.h"
#include "core/include/model_params.h"
#include "core/include/file_lmp.h"
#include "core/include/group.h"
#include "core/include/particles.h"
#include "core/include/select.h"
#include "core/include/select_position.h"
#include "core/include/visit_particles.h"
#include "core/include/configuration.h"
#include "core/include/file_xyz.h"
#include "core/include/model.h"
#include "core/include/visit_model.h"
#include "core/include/visit_model_intra.h"
#include "core/include/select_list.h"
#include "core/include/model_two_body.h"
#include "core/include/model_hard_sphere.h"
#include "core/include/model_lj.h"
#include "core/include/system.h"
#include "core/include/criteria.h"
#include "core/include/criteria_mayer.h"
#include "core/include/criteria_metropolis.h"
#include "core/include/perturb.h"
#include "core/include/perturb_transfer.h"
#include "core/include/trial.h"
#include "core/include/trial_factory.h"
#include "core/include/trial_transfer.h"
#include "core/include/macrostate.h"
#include "core/include/macrostate_num_particles.h"
#include "core/include/criteria_flat_histogram.h"
#include "core/include/model_one_body.h"
#include "core/include/model_lrc.h"
#include "core/include/perturb_translate.h"
#include "core/include/trial_translate.h"
#include "core/include/visit_configuration.h"
#include "ewald/include/ewald.h"
#include "ewald/include/model_charge_real.h"
#include "confinement/include/shape.h"
#include "confinement/include/model_hard_shape.h"
#include "example/include/model_example.h"
using namespace feasst;
%}

%include "std_string.i"
%include "std_vector.i"
%include "std_shared_ptr.i"
%include "std_iostream.i"
%template(IntVector) std::vector<int>;
%template(DoubleVector) std::vector<double>;
using namespace std;
%pythonnondynamic;

%include core/include/file.h
%include core/include/accumulator.h
%include core/include/properties.h
%include core/include/typed_entity.h
%include core/include/utils_math.h
%include core/include/position.h
%include core/include/visit_bond.h
%include core/include/formula.h
%include core/include/formula_exponential.h
%include core/include/histogram.h
%include core/include/formula_polynomial.h
%include core/include/bias.h
%include core/include/bias_wang_landau.h
%include core/include/bias_transition_matrix.h
%include core/include/custom_exception.h
%include core/include/cells.h
%include core/include/utils.h
%include core/include/debug.h
%include core/include/random.h
%include core/include/domain.h
%include core/include/domain_triclinic.h
%include core/include/domain_cuboid.h
%include core/include/physical_constants.h
%include core/include/constants.h
%include core/include/bond.h
%include core/include/utils_io.h
%include core/include/site.h
%include core/include/particle.h
%include core/include/model_params.h
%include core/include/file_lmp.h
%include core/include/group.h
%include core/include/particles.h
%include core/include/select.h
%include core/include/select_position.h
%include core/include/visit_particles.h
%include core/include/configuration.h
%include core/include/file_xyz.h
%include core/include/model.h
%include core/include/visit_model.h
%include core/include/visit_model_intra.h
%include core/include/select_list.h
%include core/include/model_two_body.h
%include core/include/model_hard_sphere.h
%include core/include/model_lj.h
%include core/include/system.h
%include core/include/criteria.h
%include core/include/criteria_mayer.h
%include core/include/criteria_metropolis.h
%include core/include/perturb.h
%include core/include/perturb_transfer.h
%include core/include/trial.h
%include core/include/trial_factory.h
%include core/include/trial_transfer.h
%include core/include/macrostate.h
%include core/include/macrostate_num_particles.h
%include core/include/criteria_flat_histogram.h
%include core/include/model_one_body.h
%include core/include/model_lrc.h
%include core/include/perturb_translate.h
%include core/include/trial_translate.h
%include core/include/visit_configuration.h
%include ewald/include/ewald.h
%include ewald/include/model_charge_real.h
%include confinement/include/shape.h
%include confinement/include/model_hard_shape.h
%include example/include/model_example.h
