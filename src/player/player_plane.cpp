#include "player/player_plane.hpp"
#include "player/player_direction_reset.hpp"
namespace sadx {
bool source_planes_parallel(const NinjaPlane& first,const NinjaPlane& second,
        const PlayerPlaneConstants& constants,X87Context& fp) {
    // Source795823/29/2d cached FLDs precede the dot-product spill.
    const auto z=fp.from_float(second.normal.z);
    const auto y=fp.from_float(second.normal.y);
    const auto x=fp.from_float(second.normal.x);
    const auto first_normal=first.normal; // source MOV snapshots
    const auto zz=fp.multiply_float_memory(fp.from_float(second.normal.z),first.normal.z);
    const auto yy=fp.multiply_float_memory(fp.from_float(second.normal.y),first.normal.y);
    const auto zy=fp.add(zz,yy);
    const auto xx=fp.multiply_float_memory(fp.from_float(first.normal.x),second.normal.x);
    const float dot=fp.to_float(fp.add(zy,xx));
    const auto x2=fp.multiply(x,x),y2=fp.multiply(y,y);
    const auto xy=fp.add(x2,y2);
    const float second_square=fp.to_float(fp.add(xy,fp.multiply(z,z)));
    const float second_length=fp.to_float(signed_sqrt_extended(second_square,fp));
    const auto a=fp.multiply_float_memory(fp.from_float(first_normal.x),first_normal.x);
    const auto b=fp.multiply_float_memory(fp.from_float(first_normal.y),first_normal.y);
    const auto ab=fp.add(a,b);
    const auto c=fp.multiply_float_memory(fp.from_float(first_normal.z),first_normal.z);
    const float first_square=fp.to_float(fp.add(ab,c));
    const auto denominator=fp.multiply_float_memory(signed_sqrt_extended(first_square,fp),second_length);
    const auto ratio=fp.reverse_divide_float_memory(denominator,dot);
    const auto cosine=fp.from_canonical_ext80(ratio.sign_exponent()&0x7fffU,ratio.significand());
    return fp.compare_float_memory(cosine,constants.parallel_cosine)==X87Comparison::greater;
}
X87Value intersect_source_planes(const NinjaPlane& first,const NinjaPlane& second,NinjaPlane* output,
        const PlayerPlaneConstants& constants,X87Context& fp) {
    if(source_planes_parallel(first,second,constants,fp)) {
        const auto point=first.point;
        if(output)*output={{0,0,0},{0,0,0}};
        return project_point_onto_plane(point,second.point,second.normal,nullptr,constants.distance_scale,fp);
    }
    if(!output)return fp.from_float(constants.axis.zero);
    // Static lift from REA evidence SHA256 f0744ac028cb7c1fa1797ab5f39b68ff4143ce70bbe19356a80d01a1d5920645
    const auto v0=fp.from_float(second.normal.z); // 0x788831
    const auto v1=fp.multiply_float_memory(v0,first.normal.y); // 0x788834
    const auto v2=fp.from_float(second.normal.y); // 0x788837
    const auto v3=fp.multiply_float_memory(v2,first.normal.z); // 0x78883a
    const auto v4=fp.subtract(v1,v3); // 0x78883d
    output->normal.x=fp.to_float(v4); // 0x78883f
    const auto v5=fp.from_float(first.normal.z); // 0x788842
    const auto v6=fp.multiply_float_memory(v5,second.normal.x); // 0x788845
    const auto v7=fp.from_float(second.normal.z); // 0x788848
    const auto v8=fp.multiply_float_memory(v7,first.normal.x); // 0x78884b
    const auto v9=fp.subtract(v6,v8); // 0x78884e
    output->normal.y=fp.to_float(v9); // 0x788850
    const auto v10=fp.from_float(second.normal.y); // 0x788853
    const auto v11=fp.multiply_float_memory(v10,first.normal.x); // 0x788856
    const auto v12=fp.from_float(first.normal.y); // 0x788859
    const auto v13=fp.multiply_float_memory(v12,second.normal.x); // 0x78885c
    const auto v14=fp.subtract(v11,v13); // 0x78885f
    output->normal.z=fp.to_float(v14); // 0x788861
    const auto v15=fp.from_float(first.point.z); // 0x788864
    const auto v16=fp.multiply_float_memory(v15,first.normal.z); // 0x788867
    const auto v17=fp.from_float(first.point.y); // 0x78886a
    const auto v18=fp.multiply_float_memory(v17,first.normal.y); // 0x78886d
    const auto v19=fp.add(v16,v18); // 0x788870
    const auto v20=fp.from_float(first.point.x); // 0x788872
    const auto v21=fp.multiply_float_memory(v20,first.normal.x); // 0x788874
    const auto v22=fp.add(v19,v21); // 0x788877
    const auto v23=fp.from_float(second.normal.x); // 0x788879
    const auto v24=fp.multiply_float_memory(v23,second.point.x); // 0x78887c
    const auto v25=fp.from_float(second.point.y); // 0x78887e
    const auto v26=fp.multiply_float_memory(v25,second.normal.y); // 0x788881
    const auto v27=fp.add(v24,v26); // 0x788884
    const auto v28=fp.from_float(second.normal.z); // 0x788886
    const auto v29=fp.multiply_float_memory(v28,second.point.z); // 0x788889
    const auto v30=fp.add(v27,v29); // 0x78888c
    const auto v31=fp.multiply_float_memory(v14,second.point.z); // 0x788890
    const auto v32=fp.from_float(second.point.y); // 0x788893
    const auto v33=fp.multiply_float_memory(v32,output->normal.y); // 0x788896
    const auto v34=fp.add(v31,v33); // 0x788899
    const auto v35=fp.from_float(output->normal.x); // 0x78889b
    const auto v36=fp.multiply_float_memory(v35,second.point.x); // 0x78889e
    const auto v37=fp.add(v34,v36); // 0x7888a0
    const auto v38=fp.from_float(second.normal.y); // 0x7888a2
    const auto v39=fp.multiply_float_memory(v38,first.normal.x); // 0x7888a5
    const auto v40=fp.from_float(first.normal.z); // 0x7888a8
    const auto v41=fp.multiply_float_memory(v40,second.normal.x); // 0x7888ab
    const auto v42=fp.from_float(second.normal.z); // 0x7888ae
    const auto v43=fp.multiply_float_memory(v42,first.normal.y); // 0x7888b1
    float spill_var_14=fp.to_float(v43); // 0x7888b4
    const auto v44=fp.from_float(second.normal.y); // 0x7888b8
    const auto v45=fp.multiply_float_memory(v44,first.normal.z); // 0x7888bb
    float spill_var_10=fp.to_float(v45); // 0x7888be
    const auto v46=fp.from_float(first.normal.y); // 0x7888c2
    const auto v47=fp.multiply_float_memory(v46,second.normal.x); // 0x7888c5
    float spill_var_18=fp.to_float(v47); // 0x7888c8
    const auto v48=fp.from_float(second.normal.z); // 0x7888cc
    const auto v49=fp.multiply_float_memory(v48,first.normal.x); // 0x7888cf
    float spill_var_1c=fp.to_float(v49); // 0x7888d2
    const auto v50=fp.multiply_float_memory(v41,output->normal.y); // 0x7888d8
    const auto v51=fp.from_float(spill_var_14); // 0x7888db
    const auto v52=fp.multiply_float_memory(v51,output->normal.x); // 0x7888df
    const auto v53=fp.add(v50,v52); // 0x7888e2
    const auto v54=fp.multiply(v14,v39); // 0x7888e6
    const auto v55=fp.add(v53,v54); // 0x7888e8
    const auto v56=fp.from_float(spill_var_10); // 0x7888ea
    const auto v57=fp.multiply_float_memory(v56,output->normal.x); // 0x7888ee
    const auto v58=fp.subtract(v55,v57); // 0x7888f1
    const auto v59=fp.multiply_float_memory(v14,spill_var_18); // 0x7888f5
    const auto v60=fp.subtract(v58,v59); // 0x7888f9
    const auto v61=fp.from_float(spill_var_1c); // 0x7888fb
    const auto v62=fp.multiply_float_memory(v61,output->normal.y); // 0x7888ff
    const auto v63=fp.subtract(v60,v62); // 0x788902
    const auto v64=fp.reverse_divide_float_memory(v63,constants.axis.one); // 0x788904
    float spill_arg_0=fp.to_float(v64); // 0x78890a
    const auto v65=fp.from_float(second.normal.z); // 0x78890e
    const auto v66=fp.multiply_float_memory(v65,output->normal.x); // 0x788911
    const auto v67=fp.multiply(v66,v22); // 0x788914
    const auto v68=fp.multiply_float_memory(v14,first.normal.x); // 0x788918
    const auto v69=fp.multiply(v68,v30); // 0x78891b
    const auto v70=fp.add(v67,v69); // 0x78891d
    const auto v71=fp.multiply(v41,v37); // 0x788921
    const auto v72=fp.add(v70,v71); // 0x788923
    const auto v73=fp.from_float(output->normal.x); // 0x788925
    const auto v74=fp.multiply_float_memory(v73,first.normal.z); // 0x788928
    const auto v75=fp.multiply(v74,v30); // 0x78892b
    const auto v76=fp.subtract(v72,v75); // 0x78892d
    const auto v77=fp.multiply_float_memory(v14,second.normal.x); // 0x788931
    const auto v78=fp.multiply(v77,v22); // 0x788934
    const auto v79=fp.subtract(v76,v78); // 0x788936
    const auto v80=fp.from_float(spill_var_1c); // 0x788938
    const auto v81=fp.multiply(v80,v37); // 0x78893c
    const auto v82=fp.subtract(v79,v81); // 0x78893e
    spill_var_1c=fp.to_float(v82); // 0x788940
    const auto v83=fp.from_float(output->normal.x); // 0x788944
    const auto v84=fp.multiply_float_memory(v83,first.normal.y); // 0x788947
    const auto v85=fp.multiply(v84,v30); // 0x78894a
    const auto v86=fp.from_float(second.normal.x); // 0x78894c
    const auto v87=fp.multiply_float_memory(v86,output->normal.y); // 0x78894f
    const auto v88=fp.multiply(v87,v22); // 0x788952
    const auto v89=fp.add(v85,v88); // 0x788954
    const auto v90=fp.multiply(v39,v37); // 0x788958
    const auto v91=fp.add(v89,v90); // 0x78895a
    const auto v92=fp.from_float(output->normal.x); // 0x78895c
    const auto v93=fp.multiply_float_memory(v92,second.normal.y); // 0x78895f
    const auto v94=fp.multiply(v93,v22); // 0x788962
    const auto v95=fp.subtract(v91,v94); // 0x788964
    const auto v96=fp.from_float(spill_var_18); // 0x788966
    const auto v97=fp.multiply(v96,v37); // 0x78896a
    const auto v98=fp.subtract(v95,v97); // 0x78896c
    const auto v99=fp.from_float(first.normal.x); // 0x78896e
    const auto v100=fp.multiply_float_memory(v99,output->normal.y); // 0x788971
    const auto v101=fp.multiply(v100,v30); // 0x788974
    const auto v102=fp.subtract(v98,v101); // 0x788976
    spill_var_18=fp.to_float(v102); // 0x788978
    const auto v103=fp.from_float(first.normal.z); // 0x78897c
    const auto v104=fp.multiply_float_memory(v103,output->normal.y); // 0x78897f
    const auto v105=fp.multiply(v104,v30); // 0x788982
    const auto v106=fp.multiply_float_memory(v14,second.normal.y); // 0x788986
    const auto v107=fp.multiply(v106,v22); // 0x788989
    const auto v108=fp.add(v105,v107); // 0x78898b
    const auto v109=fp.from_float(spill_var_14); // 0x78898d
    const auto v110=fp.multiply(v109,v37); // 0x788991
    const auto v111=fp.add(v108,v110); // 0x788993
    const auto v112=fp.from_float(spill_var_10); // 0x788995
    const auto v113=fp.multiply(v112,v37); // 0x788999
    const auto v114=fp.subtract(v111,v113); // 0x78899b
    const auto v115=fp.multiply_float_memory(v14,first.normal.y); // 0x78899f
    const auto v116=fp.multiply(v115,v30); // 0x7889a2
    const auto v117=fp.subtract(v114,v116); // 0x7889a4
    const auto v118=fp.from_float(second.normal.z); // 0x7889a6
    const auto v119=fp.multiply_float_memory(v118,output->normal.y); // 0x7889a9
    const auto v120=fp.multiply(v119,v22); // 0x7889ac
    const auto v121=fp.subtract(v117,v120); // 0x7889ae
    const auto v122=fp.multiply_float_memory(v121,spill_arg_0); // 0x7889b2
    output->point.x=fp.to_float(v122); // 0x7889b6
    const auto v123=fp.from_float(spill_var_1c); // 0x7889be
    const auto v124=fp.multiply_float_memory(v123,spill_arg_0); // 0x7889c2
    output->point.y=fp.to_float(v124); // 0x7889c6
    const auto v125=fp.from_float(spill_var_18); // 0x7889c9
    const auto v126=fp.multiply_float_memory(v125,spill_arg_0); // 0x7889cd
    output->point.z=fp.to_float(v126); // 0x7889d1
    return fp.from_float(constants.axis.zero);
}
} // namespace sadx
