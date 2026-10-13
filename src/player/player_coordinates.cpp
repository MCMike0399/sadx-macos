#include "player/player_coordinates.hpp"

#include <limits>
#include <utility>

namespace sadx {

CoordinateError::CoordinateError(CoordinateFailure failure)
    : std::runtime_error("invalid native ownership/state for source Ninja matrix stack"), failure_(failure) {}

NinjaMatrixStack::NinjaMatrixStack(std::vector<NinjaMatrix> slots, std::uint32_t source_depth,
                                   std::optional<std::uint32_t> normalization_flag)
    : slots_(std::move(slots)), depth_(source_depth), normalization_(normalization_flag) {
    if (slots_.empty() || slots_.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
        depth_ == 0 || depth_ > slots_.size()) throw CoordinateError(CoordinateFailure::invalid_matrix_stack);
}

bool NinjaMatrixStack::push_ex() {
    if (depth_ >= slots_.size()) return false;
    slots_[depth_] = current();
    ++depth_;
    return true;
}

bool NinjaMatrixStack::push_matrix(const NinjaMatrix& matrix) {
    if (depth_ >= slots_.size()) return false;
    slots_[depth_] = matrix;
    ++depth_;
    return true;
}

void NinjaMatrixStack::pop_one() {
    if (depth_ == 0) throw CoordinateError(CoordinateFailure::invalid_matrix_stack);
    if (depth_ == 1) return; // 0x785147: target depth0 does not move pointer/count
    --depth_;
}

void NinjaMatrixStack::pop_ex() { if (depth_ > 0) --depth_; }

NinjaMatrix& NinjaMatrixStack::current() {
    if (depth_ == 0) throw CoordinateError(CoordinateFailure::invalid_matrix_stack);
    return slots_[depth_ - 1];
}
const NinjaMatrix& NinjaMatrixStack::current() const {
    if (depth_ == 0) throw CoordinateError(CoordinateFailure::invalid_matrix_stack);
    return slots_[depth_ - 1];
}

PlayerCoordinates::PlayerCoordinates(NinjaMath::SineTable quarter_sine_table, NinjaMatrix unit_matrix,
                                     NinjaMatrixStack& matrix_stack)
    : table_(std::move(quarter_sine_table)), unit_(std::move(unit_matrix)), stack_(matrix_stack) {}

bool PlayerCoordinates::begin_unit_frame() {
    const bool pushed=stack_.push_ex();
    stack_.current()=unit_;
    return pushed;
}
void PlayerCoordinates::end_frame() { stack_.pop_one(); }
bool PlayerCoordinates::push_unit_frame() { return stack_.push_matrix(unit_); }
bool PlayerCoordinates::push_current_frame() { return stack_.push_ex(); }
void PlayerCoordinates::set_current_unit_matrix() { stack_.current()=unit_; }
void PlayerCoordinates::end_ex_frame() { stack_.pop_ex(); }
NinjaMatrix& PlayerCoordinates::current_matrix() { return stack_.current(); }

namespace {

std::int32_t wrapped_negation(std::int32_t angle) {
    return static_cast<std::int32_t>((0U - static_cast<std::uint32_t>(angle)) & 0xffffU);
}

std::array<float, 4> row(const NinjaMatrix& matrix, std::size_t index) {
    return {matrix[index*4],matrix[index*4+1],matrix[index*4+2],matrix[index*4+3]};
}

} // namespace

NinjaMath PlayerCoordinates::source_math(X87Context& fp) const { return NinjaMath(table_,fp); }

void PlayerCoordinates::rotate_y(NinjaMatrix& matrix, std::int32_t angle, X87Context& fp) {
    NinjaMath math(table_,fp);
    const float sine = math.sin(angle); // source binary32 spill
    const auto cosine = math.cos_extended(angle); // retained x87 register
    const auto first = row(matrix,0), second = row(matrix,2);
    const auto first0 = fp.from_float(first[0]), first1 = fp.from_float(first[1]);
    for (std::size_t i=0;i<4;++i) {
        const auto old = i==0 ? first0 : i==1 ? first1 : fp.from_float(first[i]);
        const auto a = fp.multiply(old,cosine);
        const auto b = fp.multiply_float_memory(fp.from_float(sine),second[i]);
        matrix[i] = fp.to_float(fp.subtract(a,b));
    }
    for (std::size_t i=0;i<4;++i) {
        const auto old = i==0 ? first0 : i==1 ? first1 : fp.from_float(first[i]);
        const auto a = fp.multiply_float_memory(cosine,second[i]);
        const auto b = fp.multiply_float_memory(old,sine);
        matrix[8+i] = fp.to_float(fp.add(a,b));
    }
}

void PlayerCoordinates::rotate_x(NinjaMatrix& matrix, std::int32_t angle, X87Context& fp) {
    NinjaMath math(table_,fp);
    const float sine = math.sin(angle);
    const auto cosine = math.cos_extended(angle);
    const auto first = row(matrix,1), second = row(matrix,2);
    const auto first0 = fp.from_float(first[0]), first1 = fp.from_float(first[1]);
    for (std::size_t i=0;i<4;++i) {
        const auto old = i==0 ? first0 : i==1 ? first1 : fp.from_float(first[i]);
        const auto a = fp.multiply_float_memory(fp.from_float(sine),second[i]);
        const auto b = fp.multiply(old,cosine);
        matrix[4+i] = fp.to_float(fp.add(a,b));
    }
    for (std::size_t i=0;i<4;++i) {
        const auto old = i==0 ? first0 : i==1 ? first1 : fp.from_float(first[i]);
        const auto a = fp.multiply_float_memory(cosine,second[i]);
        const auto b = fp.multiply_float_memory(old,sine);
        matrix[8+i] = fp.to_float(fp.subtract(a,b));
    }
}

void PlayerCoordinates::rotate_z(NinjaMatrix& matrix, std::int32_t angle, X87Context& fp) {
    NinjaMath math(table_,fp);
    const float sine = math.sin(angle);
    const auto cosine = math.cos_extended(angle);
    const auto first = row(matrix,0), second = row(matrix,1);
    const auto first0 = fp.from_float(first[0]), first1 = fp.from_float(first[1]);
    for (std::size_t i=0;i<4;++i) {
        const auto old = i==0 ? first0 : i==1 ? first1 : fp.from_float(first[i]);
        const auto a = fp.multiply(old,cosine);
        const auto b = fp.multiply_float_memory(fp.from_float(sine),second[i]);
        matrix[i] = fp.to_float((i&1U) ? fp.add(b,a) : fp.add(a,b));
    }
    for (std::size_t i=0;i<4;++i) {
        const auto old = i==0 ? first0 : i==1 ? first1 : fp.from_float(first[i]);
        const auto a = fp.multiply_float_memory(cosine,second[i]);
        const auto b = fp.multiply_float_memory(old,sine);
        matrix[4+i] = fp.to_float(fp.subtract(a,b));
    }
}

CoordinateResult PlayerCoordinates::calculate_vector(const NinjaMatrix& matrix, const NinjaVector& input,
                                                       NinjaVector& output, X87Context& fp) {
    const auto x = fp.from_float(input.x), y = fp.from_float(input.y), z = fp.from_float(input.z);
    for (std::size_t i=0;i<3;++i) {
        const auto a = fp.multiply_float_memory(z,matrix[8+i]);
        const auto b = fp.multiply_float_memory(y,matrix[4+i]);
        const auto ab=fp.add(a,b);
        const auto c = fp.multiply_float_memory(x,matrix[i]);
        const float result = fp.to_float(fp.add(ab,c));
        if (i==0) output.x=result;
        else if (i==1) output.y=result;
        else output.z=result;
    }
    const auto normalization = stack_.normalization_flag();
    if (!normalization) return {CoordinateFailure::unknown_normalization_flag,0x784db9};
    if (*normalization == 0) return {};
    const NinjaVector saved = output;
    // Source normalization sums z²+y²+x², stores binary32 before signed sqrt.
    const float squared = fp.to_float(inner_product_extended(saved,saved,fp));
    // signed_sqrt_extended owns the source FCOM; do not repeat it here.
    const auto inverse = fp.reverse_divide_float_memory(signed_sqrt_extended(squared,fp),1.0F);
    output.x=fp.to_float(fp.multiply(fp.from_float(saved.x),inverse));
    output.y=fp.to_float(fp.multiply(fp.from_float(saved.y),inverse));
    output.z=fp.to_float(fp.multiply_float_memory(inverse,saved.z));
    return {};
}

CoordinateResult PlayerCoordinates::world_to_player(const PlayerOrientation& task, NinjaVector& vector,
                                                      X87Context& fp) {
    const NinjaVector saved = vector;
    (void)begin_unit_frame();
    if (task.angles[1] != 0) rotate_y(stack_.current(),task.angles[1]&0xffff,fp);
    if (task.angles[0] != 0) rotate_x(stack_.current(),wrapped_negation(task.angles[0]),fp);
    if (task.angles[2] != 0) rotate_z(stack_.current(),wrapped_negation(task.angles[2]),fp);
    const auto calculated=calculate_vector(stack_.current(),saved,vector,fp);
    if (!calculated) return calculated; // retained partial stores/stack ownership
    end_frame();
    return {};
}

CoordinateResult PlayerCoordinates::task_to_world(const PlayerOrientation* task, NinjaVector& vector,
                                                  X87Context& fp) {
    if (!task) return {}; // actual null-task branch, no matrix or vector change
    const NinjaVector saved=vector;
    (void)begin_unit_frame();
    if (task->angles[2] != 0) rotate_z(stack_.current(),task->angles[2]&0xffff,fp);
    if (task->angles[0] != 0) rotate_x(stack_.current(),task->angles[0]&0xffff,fp);
    if (task->angles[1] != 0) rotate_y(stack_.current(),wrapped_negation(task->angles[1]),fp);
    const auto calculated=calculate_vector(stack_.current(),saved,vector,fp);
    if (!calculated) return calculated;
    end_frame();
    return {};
}

CoordinateResult PlayerCoordinates::floor_to_world(const NinjaVector& normal, NinjaVector& vector,
                                                   std::int32_t yaw, const NormalAnglesProvider& provider,
                                                   X87Context& fp) {
    // 43edd8..43ede0/43eded..43edf1 capture the input before 43ee05's
    // angle-helper call, including when its native binding observes live state.
    const NinjaVector saved=vector;
    if (!provider) return {CoordinateFailure::missing_normal_angles,0x43ee05};
    const auto decoded=provider(normal,fp); // occurs BEFORE pushing matrix stack
    if (decoded.failure != CoordinateFailure::none) return {decoded.failure,0x43ee05};
    if (!decoded.angles) return {CoordinateFailure::normal_angles_unavailable,0x43ee05};
    (void)begin_unit_frame();
    if (decoded.angles->roll != 0) rotate_z(stack_.current(),decoded.angles->roll&0xffff,fp);
    if (decoded.angles->inclination != 0) rotate_x(stack_.current(),decoded.angles->inclination&0xffff,fp);
    if (yaw != 0) rotate_y(stack_.current(),wrapped_negation(yaw),fp);
    const auto calculated=calculate_vector(stack_.current(),saved,vector,fp);
    if (!calculated) return calculated;
    end_frame();
    return {};
}

void PlayerCoordinates::rotate_axis(NinjaMatrix& matrix,const NinjaVector& input_axis,
    std::int32_t angle,const CoordinateAxisConstants& constants,X87Context& fp) {
    const NinjaVector axis=input_axis; // source copies all three input words before stores
    const auto z=fp.multiply_float_memory(fp.from_float(axis.z),axis.z);
    const auto y=fp.multiply_float_memory(fp.from_float(axis.y),axis.y);
    const float squared=fp.to_float(fp.add(z,y));
    const auto length=signed_sqrt_extended(squared,fp);
    const auto equals_zero=[&](float value) {
        const auto zero=fp.from_float(constants.zero);
        const auto loaded=fp.from_float(value);
        return fp.compare_quiet(loaded,zero)==X87Comparison::equal;
    };
    const bool z_zero=equals_zero(axis.z);
    const bool y_nonzero=!equals_zero(axis.y);
    float sc,s14;
    if(z_zero&&y_nonzero) {
        s14=fp.compare_float_memory(fp.from_float(axis.y),constants.zero)==X87Comparison::greater?1.0F:-1.0F;
        sc=0.0F;
    } else if(equals_zero(axis.y)) {
        s14=0.0F;
        sc=fp.compare_float_memory(fp.from_float(axis.z),constants.zero)==X87Comparison::greater?1.0F:-1.0F;
    } else {
        const auto inverse=fp.divide(fp.from_float(constants.one),length);
        s14=fp.to_float(fp.multiply(fp.from_float(axis.y),inverse));
        sc=fp.to_float(fp.multiply_float_memory(inverse,axis.z));
    }
    const auto loaded_zero=fp.from_float(constants.zero);
    const bool length_zero=fp.compare_quiet(length,loaded_zero)==X87Comparison::equal;
    const bool x_nonzero=!equals_zero(axis.x);
    float s8,s10;
    if(length_zero&&x_nonzero) {
        s10=fp.compare_float_memory(fp.from_float(axis.x),constants.zero)==X87Comparison::greater?1.0F:-1.0F;
        s8=0.0F;
    } else if(equals_zero(axis.x)) {
        s10=0.0F;
        s8=fp.compare_float_memory(length,constants.zero)==X87Comparison::greater?1.0F:-1.0F;
    } else {
        s8=fp.to_float(length);
        s10=axis.x; // source MOV, not another FP store or full-axis normalization
    }
    NinjaMath math(table_,fp);
    // Source0x788fa0..0x789314 lifted to straight-line native expressions.
    // No guest instruction interpreter/register stack exists at runtime.
    const auto v1=fp.from_float(matrix[4]); // 0x788fa0
    const auto v2=matrix[6]; // 0x788fa3
    const auto v3=fp.from_float(matrix[5]); // 0x788fa6
    const auto v4=matrix[7]; // 0x788fa9
    const auto v5=fp.from_float(s14); // 0x788fac
    float s20=v2; // 0x788fb0
    const auto v6=v5.negated(); // 0x788fb4
    float s24=v4; // 0x788fb6
    float s18=fp.to_float(v6); // 0x788fba
    const auto v7=matrix[2]; // 0x788fbe
    const auto v8=fp.multiply_float_memory(v6,matrix[8]); // 0x788fc1
    const auto v9=matrix[3]; // 0x788fc4
    float s1c=v9; // 0x788fc9
    const auto v10=fp.multiply_float_memory(v1,sc); // 0x788fcd
    const auto v11=fp.add(v8,v10); // 0x788fd1
    matrix[4]=fp.to_float(v11); // 0x788fd3
    const auto v12=fp.from_float(s18); // 0x788fd6
    const auto v13=fp.multiply_float_memory(v12,matrix[9]); // 0x788fda
    const auto v14=fp.multiply_float_memory(v3,sc); // 0x788fdf
    const auto v15=fp.add(v13,v14); // 0x788fe3
    matrix[5]=fp.to_float(v15); // 0x788fe5
    const auto v16=fp.from_float(s18); // 0x788fe8
    const auto v17=fp.multiply_float_memory(v16,matrix[10]); // 0x788fec
    const auto v18=fp.from_float(s20); // 0x788fef
    const auto v19=fp.multiply_float_memory(v18,sc); // 0x788ff3
    const auto v20=fp.add(v17,v19); // 0x788ff7
    matrix[6]=fp.to_float(v20); // 0x788ff9
    const auto v21=fp.from_float(s18); // 0x788ffc
    const auto v22=fp.multiply_float_memory(v21,matrix[11]); // 0x789000
    const auto v23=fp.from_float(s24); // 0x789003
    const auto v24=fp.multiply_float_memory(v23,sc); // 0x789007
    const auto v25=fp.add(v22,v24); // 0x78900b
    matrix[7]=fp.to_float(v25); // 0x78900d
    const auto v26=fp.from_float(sc); // 0x789010
    const auto v27=fp.multiply_float_memory(v26,matrix[8]); // 0x789014
    const auto v28=fp.from_float(s18); // 0x789017
    const auto v29=fp.multiply(v28,v1); // 0x78901b
    const auto v30=fp.subtract(v27,v29); // 0x78901d
    float s28=fp.to_float(v30); // 0x78901f
    const auto v31=fp.from_float(sc); // 0x789023
    const auto v32=fp.multiply_float_memory(v31,matrix[9]); // 0x789027
    const auto v33=fp.from_float(s18); // 0x78902a
    const auto v34=fp.multiply(v33,v3); // 0x78902e
    const auto v35=fp.subtract(v32,v34); // 0x789030
    const auto v36=fp.from_float(sc); // 0x789036
    const auto v37=fp.multiply_float_memory(v36,matrix[10]); // 0x78903a
    const auto v38=fp.from_float(s18); // 0x78903d
    const auto v39=fp.multiply_float_memory(v38,s20); // 0x789041
    const auto v40=fp.subtract(v37,v39); // 0x789045
    const auto v41=fp.from_float(sc); // 0x789047
    const auto v42=fp.multiply_float_memory(v41,matrix[11]); // 0x78904b
    const auto v43=fp.from_float(s18); // 0x78904e
    s18=v7; // 0x789052
    const auto v44=fp.multiply_float_memory(v43,s24); // 0x789056
    const auto v45=fp.subtract(v42,v44); // 0x78905a
    float s2c=fp.to_float(v45); // 0x78905c
    const auto v46=fp.from_float(matrix[0]); // 0x789060
    const auto v47=fp.from_float(matrix[1]); // 0x789062
    const auto v48=fp.multiply_float_memory(v46,s8); // 0x789067
    const auto v49=fp.from_float(s28); // 0x78906b
    const auto v50=fp.multiply_float_memory(v49,s10); // 0x78906f
    const auto v51=fp.subtract(v48,v50); // 0x789073
    matrix[0]=fp.to_float(v51); // 0x789075
    const auto v52=fp.from_float(s8); // 0x789077
    const auto v53=fp.multiply(v52,v47); // 0x78907b
    const auto v54=fp.multiply_float_memory(v35,s10); // 0x78907f
    const auto v55=fp.subtract(v53,v54); // 0x789083
    matrix[1]=fp.to_float(v55); // 0x789085
    const auto v56=fp.from_float(s18); // 0x789088
    const auto v57=fp.multiply_float_memory(v56,s8); // 0x78908c
    const auto v58=fp.multiply_float_memory(v40,s10); // 0x789092
    const auto v59=fp.subtract(v57,v58); // 0x789096
    matrix[2]=fp.to_float(v59); // 0x789098
    const auto v60=fp.from_float(s1c); // 0x78909b
    const auto v61=fp.multiply_float_memory(v60,s8); // 0x78909f
    const auto v62=fp.from_float(s2c); // 0x7890a7
    const auto v63=fp.multiply_float_memory(v62,s10); // 0x7890ad
    const auto v64=fp.subtract(v61,v63); // 0x7890b1
    matrix[3]=fp.to_float(v64); // 0x7890b3
    const auto v65=fp.from_float(s28); // 0x7890b6
    const auto v66=fp.multiply_float_memory(v65,s8); // 0x7890ba
    const auto v67=fp.multiply_float_memory(v46,s10); // 0x7890c0
    const auto v68=fp.add(v66,v67); // 0x7890c4
    matrix[8]=fp.to_float(v68); // 0x7890c8
    const auto v69=fp.multiply_float_memory(v35,s8); // 0x7890cd
    const auto v70=fp.multiply_float_memory(v47,s10); // 0x7890d3
    const auto v71=fp.add(v69,v70); // 0x7890d7
    matrix[9]=fp.to_float(v71); // 0x7890db
    const auto v72=fp.multiply_float_memory(v40,s8); // 0x7890de
    const auto v73=fp.from_float(s18); // 0x7890e2
    const auto v74=fp.multiply_float_memory(v73,s10); // 0x7890e6
    const auto v75=fp.add(v72,v74); // 0x7890ea
    matrix[10]=fp.to_float(v75); // 0x7890ec
    const auto v76=fp.from_float(s2c); // 0x7890ef
    const auto v77=fp.multiply_float_memory(v76,s8); // 0x7890f3
    const auto v78=fp.from_float(s1c); // 0x7890f7
    const auto v79=fp.multiply_float_memory(v78,s10); // 0x7890fb
    const auto v80=fp.add(v77,v79); // 0x7890ff
    matrix[11]=fp.to_float(v80); // 0x789101
    const auto v81=math.sin_extended(angle); // 0x789104
    float s34=fp.to_float(v81); // 0x78910b
    const auto v82=math.cos_extended(angle); // 0x78910f
    const auto v83=fp.from_float(matrix[0]); // 0x789114
    const auto v84=fp.from_float(matrix[1]); // 0x789116
    const auto v85=matrix[2]; // 0x789119
    const auto v86=fp.from_float(s34); // 0x78911c
    const auto v87=matrix[3]; // 0x789120
    const auto v88=fp.multiply_float_memory(v86,matrix[4]); // 0x789123
    s18=v85; // 0x789126
    s1c=v87; // 0x78912c
    const auto v89=fp.multiply(v82,v83); // 0x789130
    const auto v90=fp.add(v88,v89); // 0x789132
    matrix[0]=fp.to_float(v90); // 0x789134
    const auto v91=fp.from_float(s34); // 0x789136
    const auto v92=fp.multiply_float_memory(v91,matrix[5]); // 0x78913a
    const auto v93=fp.multiply(v82,v84); // 0x78913f
    const auto v94=fp.add(v92,v93); // 0x789141
    matrix[1]=fp.to_float(v94); // 0x789143
    const auto v95=fp.multiply_float_memory(v82,s18); // 0x789148
    const auto v96=fp.from_float(s34); // 0x78914c
    const auto v97=fp.multiply_float_memory(v96,matrix[6]); // 0x789150
    const auto v98=fp.add(v95,v97); // 0x789153
    matrix[2]=fp.to_float(v98); // 0x789155
    const auto v99=fp.from_float(s34); // 0x789158
    const auto v100=fp.multiply_float_memory(v99,matrix[7]); // 0x78915c
    const auto v101=fp.multiply_float_memory(v82,s1c); // 0x789161
    const auto v102=fp.add(v100,v101); // 0x789165
    matrix[3]=fp.to_float(v102); // 0x789167
    const auto v103=fp.multiply_float_memory(v82,matrix[4]); // 0x78916c
    const auto v104=fp.from_float(s34); // 0x78916f
    const auto v105=fp.multiply(v104,v83); // 0x789173
    const auto v106=fp.subtract(v103,v105); // 0x789175
    matrix[4]=fp.to_float(v106); // 0x789177
    const auto v107=fp.multiply_float_memory(v82,matrix[5]); // 0x78917c
    const auto v108=fp.from_float(s34); // 0x78917f
    const auto v109=fp.multiply(v108,v84); // 0x789183
    const auto v110=fp.subtract(v107,v109); // 0x789185
    matrix[5]=fp.to_float(v110); // 0x789187
    const auto v111=fp.multiply_float_memory(v82,matrix[6]); // 0x789190
    const auto v112=matrix[2]; // 0x789193
    const auto v113=fp.from_float(s34); // 0x789196
    const auto v114=matrix[3]; // 0x78919a
    const auto v115=fp.multiply_float_memory(v113,s18); // 0x78919d
    s18=v112; // 0x7891a1
    const auto v116=fp.subtract(v111,v115); // 0x7891a5
    matrix[6]=fp.to_float(v116); // 0x7891a7
    const auto v117=matrix[6]; // 0x7891aa
    s20=v117; // 0x7891ad
    const auto v118=fp.multiply_float_memory(v82,matrix[7]); // 0x7891b1
    const auto v119=fp.from_float(s34); // 0x7891b4
    const auto v120=fp.multiply_float_memory(v119,s1c); // 0x7891b8
    s1c=v114; // 0x7891bc
    const auto v121=fp.subtract(v118,v120); // 0x7891c0
    matrix[7]=fp.to_float(v121); // 0x7891c2
    const auto v122=matrix[7]; // 0x7891c5
    const auto v123=fp.from_float(matrix[0]); // 0x7891c8
    s24=v122; // 0x7891ca
    const auto v124=fp.from_float(matrix[1]); // 0x7891ce
    const auto v125=fp.from_float(s10); // 0x7891d1
    const auto v126=v125.negated(); // 0x7891d5
    s34=fp.to_float(v126); // 0x7891d7
    const auto v127=fp.multiply_float_memory(v123,s8); // 0x7891dd
    const auto v128=fp.from_float(s34); // 0x7891e1
    const auto v129=fp.multiply_float_memory(v128,matrix[8]); // 0x7891e5
    const auto v130=fp.subtract(v127,v129); // 0x7891e8
    matrix[0]=fp.to_float(v130); // 0x7891ea
    const auto v131=fp.from_float(s8); // 0x7891ec
    const auto v132=fp.multiply(v131,v124); // 0x7891f0
    const auto v133=fp.from_float(s34); // 0x7891f2
    const auto v134=fp.multiply_float_memory(v133,matrix[9]); // 0x7891f6
    const auto v135=fp.subtract(v132,v134); // 0x7891f9
    matrix[1]=fp.to_float(v135); // 0x7891fb
    const auto v136=fp.from_float(s18); // 0x7891fe
    const auto v137=fp.multiply_float_memory(v136,s8); // 0x789202
    const auto v138=fp.from_float(s34); // 0x789206
    const auto v139=fp.multiply_float_memory(v138,matrix[10]); // 0x78920a
    const auto v140=fp.subtract(v137,v139); // 0x78920d
    matrix[2]=fp.to_float(v140); // 0x78920f
    const auto v141=fp.from_float(s1c); // 0x789212
    const auto v142=fp.multiply_float_memory(v141,s8); // 0x789216
    const auto v143=fp.from_float(s34); // 0x78921a
    const auto v144=fp.multiply_float_memory(v143,matrix[11]); // 0x78921e
    const auto v145=fp.subtract(v142,v144); // 0x789221
    matrix[3]=fp.to_float(v145); // 0x789223
    const auto v146=fp.from_float(s34); // 0x789226
    const auto v147=fp.multiply(v146,v123); // 0x78922a
    const auto v148=fp.from_float(s8); // 0x78922c
    const auto v149=fp.multiply_float_memory(v148,matrix[8]); // 0x789230
    const auto v150=fp.add(v147,v149); // 0x789233
    matrix[8]=fp.to_float(v150); // 0x789235
    const auto v151=fp.from_float(s34); // 0x789238
    const auto v152=fp.multiply(v151,v124); // 0x78923c
    const auto v153=fp.from_float(s8); // 0x78923e
    const auto v154=fp.multiply_float_memory(v153,matrix[9]); // 0x789242
    const auto v155=fp.add(v152,v154); // 0x789245
    matrix[9]=fp.to_float(v155); // 0x789247
    const auto v156=fp.from_float(s34); // 0x78924e
    const auto v157=fp.multiply_float_memory(v156,s18); // 0x789252
    const auto v158=fp.from_float(s8); // 0x789256
    const auto v159=fp.multiply_float_memory(v158,matrix[10]); // 0x78925a
    const auto v160=fp.add(v157,v159); // 0x78925d
    matrix[10]=fp.to_float(v160); // 0x78925f
    const auto v161=fp.from_float(s34); // 0x789262
    const auto v162=fp.multiply_float_memory(v161,s1c); // 0x789266
    const auto v163=fp.from_float(s8); // 0x78926a
    const auto v164=fp.multiply_float_memory(v163,matrix[11]); // 0x78926e
    const auto v165=fp.add(v162,v164); // 0x789271
    matrix[11]=fp.to_float(v165); // 0x789273
    const auto v166=fp.from_float(matrix[4]); // 0x789276
    const auto v167=fp.from_float(matrix[5]); // 0x789279
    const auto v168=fp.from_float(s14); // 0x78927c
    const auto v169=fp.multiply_float_memory(v168,matrix[8]); // 0x789280
    const auto v170=fp.multiply_float_memory(v166,sc); // 0x789285
    const auto v171=fp.add(v169,v170); // 0x789289
    matrix[4]=fp.to_float(v171); // 0x78928b
    const auto v172=fp.from_float(s14); // 0x78928e
    const auto v173=fp.multiply_float_memory(v172,matrix[9]); // 0x789293
    const auto v174=fp.multiply_float_memory(v167,sc); // 0x789298
    const auto v175=fp.add(v173,v174); // 0x78929c
    matrix[5]=fp.to_float(v175); // 0x78929e
    const auto v176=fp.from_float(s20); // 0x7892a1
    const auto v177=fp.multiply_float_memory(v176,sc); // 0x7892a5
    const auto v178=fp.from_float(s14); // 0x7892a9
    const auto v179=fp.multiply_float_memory(v178,matrix[10]); // 0x7892ad
    const auto v180=fp.add(v177,v179); // 0x7892b0
    matrix[6]=fp.to_float(v180); // 0x7892b2
    const auto v181=fp.from_float(s14); // 0x7892b5
    const auto v182=fp.multiply_float_memory(v181,matrix[11]); // 0x7892b9
    const auto v183=fp.from_float(s24); // 0x7892bc
    const auto v184=fp.multiply_float_memory(v183,sc); // 0x7892c0
    const auto v185=fp.add(v182,v184); // 0x7892c4
    matrix[7]=fp.to_float(v185); // 0x7892c6
    const auto v186=fp.from_float(sc); // 0x7892c9
    const auto v187=fp.multiply_float_memory(v186,matrix[8]); // 0x7892cd
    const auto v188=fp.multiply_float_memory(v166,s14); // 0x7892d2
    const auto v189=fp.subtract(v187,v188); // 0x7892d6
    matrix[8]=fp.to_float(v189); // 0x7892da
    const auto v190=fp.from_float(sc); // 0x7892dd
    const auto v191=fp.multiply_float_memory(v190,matrix[9]); // 0x7892e1
    const auto v192=fp.multiply_float_memory(v167,s14); // 0x7892e6
    const auto v193=fp.subtract(v191,v192); // 0x7892ea
    matrix[9]=fp.to_float(v193); // 0x7892ec
    const auto v194=fp.from_float(sc); // 0x7892ef
    const auto v195=fp.multiply_float_memory(v194,matrix[10]); // 0x7892f3
    const auto v196=fp.from_float(s20); // 0x7892f6
    const auto v197=fp.multiply_float_memory(v196,s14); // 0x7892fa
    const auto v198=fp.subtract(v195,v197); // 0x7892fe
    matrix[10]=fp.to_float(v198); // 0x789300
    const auto v199=fp.from_float(sc); // 0x789303
    const auto v200=fp.multiply_float_memory(v199,matrix[11]); // 0x789307
    const auto v201=fp.from_float(s24); // 0x78930a
    const auto v202=fp.multiply_float_memory(v201,s14); // 0x78930e
    const auto v203=fp.subtract(v200,v202); // 0x789312
    matrix[11]=fp.to_float(v203); // 0x789314
}

FloorProjectionResult PlayerCoordinates::project_input_to_floor(const PlayerOrientation& task,
    const NinjaVector& normal,std::int32_t raw_angle,std::int32_t camera_pitch,
    std::int32_t camera_yaw,X87Value retained,const CoordinateAxisConstants& constants,X87Context& fp) {
    FloorProjectionResult result;
    NinjaVector camera_input{fp.to_float(retained),fp.to_float(retained),-1.0F};
    (void)begin_unit_frame();
    if(camera_yaw!=0)rotate_y(stack_.current(),camera_yaw&0xffff,fp);
    if(camera_pitch!=0)rotate_x(stack_.current(),camera_pitch&0xffff,fp);
    NinjaVector camera_output;
    result.coordinate=calculate_vector(stack_.current(),camera_input,camera_output,fp);
    result.camera_direction=camera_output;
    if(!result.coordinate){result.source_boundary=0x44b4a8;return result;}
    end_frame();
    (void)begin_unit_frame();
    rotate_axis(stack_.current(),normal,raw_angle,constants,fp);
    NinjaVector projected;
    result.coordinate=calculate_vector(stack_.current(),camera_output,projected,fp);
    result.projected_direction=projected;
    if(!result.coordinate){result.source_boundary=0x44b4db;return result;}
    end_frame();
    NinjaVector forward{1.0F,0.0F,0.0F};
    result.coordinate=task_to_world(&task,forward,fp);
    result.forward_direction=forward;
    if(!result.coordinate)result.source_boundary=0x44b508;
    return result;
}

// Source0x781c10 loads x,y,z before any matrix store. Retain those
// values, including when the caller's input overlaps subsequently changed data.
void PlayerCoordinates::translate_matrix(NinjaMatrix& m,const NinjaVector& input,X87Context& fp) {
    const auto x=fp.from_float(input.x),y=fp.from_float(input.y),z=fp.from_float(input.z);
    for(unsigned column=0;column<4;++column) {
        const auto zz=fp.multiply_float_memory(z,m[8+column]);
        const auto yy=fp.multiply_float_memory(y,m[4+column]);
        const auto sum=fp.add(zz,yy);
        const auto xx=fp.multiply_float_memory(x,m[column]);
        m[12+column]=fp.to_float(fp.add_float_memory(fp.add(sum,xx),m[12+column]));
    }
}
void PlayerCoordinates::calculate_point(const NinjaMatrix& m,const NinjaVector& input,NinjaVector& out,X87Context& fp) {
    const auto x=fp.from_float(input.x),y=fp.from_float(input.y),z=fp.from_float(input.z);
    float* outputs[]={&out.x,&out.y,&out.z};
    for(unsigned column=0;column<3;++column) {
        const auto zz=fp.multiply_float_memory(z,m[8+column]);
        const auto yy=fp.multiply_float_memory(y,m[4+column]);
        const auto sum=fp.add(zz,yy);
        const auto xx=fp.multiply_float_memory(x,m[column]);
        *outputs[column]=fp.to_float(fp.add_float_memory(fp.add(sum,xx),m[12+column]));
    }
}

void PlayerCoordinates::scale_matrix(NinjaMatrix& matrix,float x,float y,float z,X87Context& fp) {
    const float scales[]={x,y,z};
    for(unsigned row=0;row<3;++row)for(unsigned column=0;column<4;++column) {
        const auto index=row*4+column;
        matrix[index]=fp.to_float(fp.multiply_float_memory(fp.from_float(scales[row]),matrix[index]));
    }
}

} // namespace sadx
