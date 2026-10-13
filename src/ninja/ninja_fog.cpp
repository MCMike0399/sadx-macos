#include "ninja/ninja_fog.hpp"
#include <cmath>
#include <cstring>
namespace sadx {
namespace {
struct Stop{};
float bits_float(std::uint32_t bits){float v;std::memcpy(&v,&bits,4);return v;}
std::uint32_t float_bits(float v){std::uint32_t b;std::memcpy(&b,&v,4);return b;}
constexpr std::uint32_t fog_color_state=34,fog_table_mode_state=35,fog_start_state=36,fog_end_state=37;
struct Operation {
    NinjaFogResult r;X87Context* context;std::uint32_t site=0; // context is null for x87-free bodies
    explicit Operation(X87Context* fp):context(fp){}
    void at(std::uint32_t s){site=s;r.last_fp_site=s;}
    [[noreturn]]void stop(NinjaFogFailure f,std::uint32_t source,std::uint32_t callee=0){r.failure=f;r.source_boundary=source;r.callee_boundary=callee;throw Stop{};}
    template<class T>T need(const std::optional<T>& v,std::uint32_t source,std::uint32_t fact){if(!v){r.required_fact=fact;stop(NinjaFogFailure::unknown_fact,source);}return *v;}
    void render_state(NinjaRenderState* render,std::uint32_t state,std::uint32_t value,NinjaRenderSourceSites sites){
        if(!render)stop(NinjaFogFailure::render_failed,sites.device_load);
        if(!ninja_issue_render_state(r.render,*render,state,value,sites))stop(NinjaFogFailure::render_failed,r.render.source_boundary);
    }
    template<class F>NinjaFogResult run(F&& body){
        try{body();}catch(Stop){}catch(const X87Error& e){r.failure=NinjaFogFailure::fp_failed;r.fp_failure=e.failure();r.fp_exceptions=e.exceptions();r.source_boundary=site;}
        return std::move(r);
    }
};
// Actual 644340 (_ftol2): FISTP m64 under the current RC, then a one-step
// correction toward zero from the FST m32 sign of x and of (x - n).
std::int64_t ftol2_644340(X87Context& fp,X87Value x){
    const auto sign=float_bits(fp.to_float(x))&0x80000000U;
    const auto n=fp.to_int64_current_rounding(x);const auto u=static_cast<std::uint64_t>(n);
    if(static_cast<std::uint32_t>(u)==0&&((u>>32)&0x7fffffffU)==0)return n; // 64439f: both values popped
    const auto d=float_bits(fp.to_float(fp.subtract(x,fp.from_int64(n))));
    if(sign){const bool carry=(d^0x80000000U)>0x80000000U;return static_cast<std::int64_t>(u+(carry?1:0));}
    const bool borrow=d>=0x80000001U;return static_cast<std::int64_t>(u-(borrow?1:0));
}
// Actual 7949f0: D3D8 table-fog render states from the current table edges,
// or the 3d128a0 fallback density when table fog is unusable.
void apply_7949f0(Operation& op,NinjaFogDeviceOwners& o){
    auto& d=o.device;auto& fp=*op.context;
    const auto recip=[&](std::uint32_t i,std::uint32_t load)->float{
        if(i>=d.reciprocal_at_3d12c00.size())op.stop(NinjaFogFailure::index_out_of_range,load);
        return op.need(d.reciprocal_at_3d12c00[i],load,0x3d12c00+4*i);};
    const auto end_index=op.need(d.end_index_at_3d0fce0,0x7949f3,0x3d0fce0);
    op.at(0x7949f8);const auto a_load=fp.from_float(recip(end_index,0x7949f8));
    const auto start_index=op.need(d.start_index_at_3d12950,0x7949ff,0x3d12950);
    const float f=op.need(d.start_value_at_3d0fac0,0x794a05,0x3d0fac0);
    op.at(0x794a0a);const float a=fp.to_float(a_load);                                        // v_C
    op.at(0x794a0e);const auto b_load=fp.from_float(recip(start_index,0x794a0e));
    const float n=op.need(d.end_value_at_3d12894,0x794a15,0x3d12894);
    op.at(0x794a1b);const float b=fp.to_float(b_load);                                        // v_10
    op.at(0x794a31);const auto dfn=fp.subtract_float_memory(fp.from_float(f),n);op.at(0x794a34);const float v14=fp.to_float(dfn);
    op.at(0x794a3c);const float v8=fp.to_float(fp.subtract_float_memory(fp.from_float(b),a));
    const auto less=[](X87Comparison c){return c==X87Comparison::less;};
    const auto f32=[&](float v){return fp.from_float(v);};
    const auto supported=op.need(o.state.fog_supported_at_3d128a4,0x794a27,0x3d128a4);
    if(supported){
        const auto linear=[&](std::uint32_t start,std::uint32_t end,NinjaRenderSourceSites first,std::uint32_t s1,std::uint32_t s2){
            op.render_state(o.render,fog_table_mode_state,3,first);
            op.render_state(o.render,fog_start_state,start,{s1,s1+5,s1+0xb});
            op.render_state(o.render,fog_end_state,end,{s2,s2+5,s2+0xb});};
        // N thresholds shared by 794b3f and 794c85: start/end words.
        const auto thresholds=[&](std::uint32_t at_low,std::uint32_t at_mid,std::uint32_t at_high,std::uint32_t& start,std::uint32_t& end){
            op.at(at_low);if(less(fp.compare_float_memory(f32(n),0.3F))){start=0x477ffe00;end=0x477fff00;return;}
            start=0;op.at(at_mid);if(!less(fp.compare_float_memory(f32(0.3F),n))){end=0x42c80000;return;}
            op.at(at_high);end=less(fp.compare_float_memory(f32(n),0.6F))?0x46fffe00:0x42c80000;};
        op.at(0x794a58);const bool near=less(fp.compare_float_memory(f32(f),0.9F));
        if(near){
            op.at(0x794a6d);if(fp.compare_float_memory(f32(v14),1.0F)==X87Comparison::greater){
                op.at(0x794a84);const auto c=fp.compare_float_memory(f32(0.5F),n);
                if(c==X87Comparison::less||c==X87Comparison::equal){linear(0,0x42c80000,{0x794a8c,0x794a93,0x794aaa},0x794ab4,0x794ac9);return;}
                // EXP2 density: the device is loaded at 794a8c and dereferenced at 794ae2 before FYL2X.
                if(!o.render)op.stop(NinjaFogFailure::render_failed,0x794a8c);
                if(!o.render->device_at_3d128b0){op.r.render.failure=NinjaRenderFailure::unknown_device;op.r.render.source_boundary=0x794a8c;op.stop(NinjaFogFailure::render_failed,0x794a8c);}
                if(!*o.render->device_at_3d128b0){op.r.render.failure=NinjaRenderFailure::null_device;op.r.render.source_boundary=0x794ae2;op.stop(NinjaFogFailure::render_failed,0x794ae2);}
                op.stop(NinjaFogFailure::unmodeled_operation,0x794afc); // FYL2X is not modeled
            }
            std::uint32_t start=0,end=0;thresholds(0x794b42,0x794b67,0x794b7c,start,end);linear(start,end,{0x794b99,0x794b9e,0x794ba5},0x794baf,0x794bc4);return;
        }
        op.at(0x794bd9);if(fp.compare_float_memory(f32(v14),0.0F)==X87Comparison::greater){
            op.at(0x794bee);const auto q=fp.divide_float_memory(f32(v8),v14);
            op.at(0x794bf7);auto t=fp.multiply(f32(n).negated(),q);op.at(0x794bf9);float start=fp.to_float(fp.add_float_memory(t,a));
            op.at(0x794c06);auto u=fp.subtract_float_memory(f32(1.0F),f);op.at(0x794c0a);u=fp.multiply(u,q);op.at(0x794c0c);float end=fp.to_float(fp.add_float_memory(u,b));
            op.at(0x794c19);if(less(fp.compare_float_memory(f32(start),0.0F)))start=0.0F;
            op.at(0x794c33);if(less(fp.compare_float_memory(f32(65535.0F),end)))end=bits_float(0x477fff00);
            linear(float_bits(start),float_bits(end),{0x794c46,0x794c4b,0x794c52},0x794c5b,0x794c70);return;
        }
        std::uint32_t start=0,end=0;thresholds(0x794c88,0x794ca4,0x794cb1,start,end);linear(start,end,{0x794c46,0x794c4b,0x794c52},0x794c5b,0x794c70);return;
    }
    // 794ce6: fallback density; D (ext F-N) is still on the stack.
    const auto fallback=[&](){
        op.at(0x794d5f);if(less(fp.compare_float_memory(f32(n),0.3F))){d.software_density_at_3d128a0=0x3f800000;return;}
        op.at(0x794d72);if(!less(fp.compare_float_memory(f32(0.3F),n))){d.software_density_at_3d128a0=0x3e99999a;return;}
        d.software_density_at_3d128a0=0x3f19999a;op.at(0x794d89);if(!less(fp.compare_float_memory(f32(n),0.6F)))d.software_density_at_3d128a0=0x3e99999a;};
    op.at(0x794cea);if(less(fp.compare_float_memory(f32(f),0.9F))){
        op.at(0x794cfd);if(fp.compare_float_memory(f32(v14),1.0F)==X87Comparison::greater){
            op.at(0x794d10);const auto c=fp.compare_float_memory(f32(0.5F),n);
            d.software_density_at_3d128a0=(c==X87Comparison::less||c==X87Comparison::equal)?0x3e99999a:0x3f800000;return;}
        fallback();return;
    }
    op.at(0x794d28);if(fp.compare_float_memory(dfn,0.0F)==X87Comparison::greater){
        op.at(0x794d39);const auto q=fp.divide(f32(v8),dfn);op.at(0x794d41);const auto u=fp.subtract_float_memory(f32(1.0F),f);
        op.at(0x794d45);const auto p=fp.multiply(q,u);op.at(0x794d47);const auto e=fp.add_float_memory(p,b);
        op.at(0x794d4b);d.software_density_at_3d128a0=less(fp.compare_float_memory(e,100.0F))?0x3e99999a:0x3f800000;return;
    }
    fallback();
}
// Actual 794db0: reciprocal table 3d12c00 from the 3d12a00 w table, then 7949f0.
void reciprocal_794db0(Operation& op,NinjaFogDeviceOwners& o,std::uint32_t p){
    auto& fp=*op.context;
    const auto shift=p&0x1fU;const auto power=static_cast<std::int32_t>(1U<<shift),fraction=static_cast<std::int32_t>(p&0xff00U);
    // TEST EAX after AND 0xff00 is never negative: the 2^32 FADD at 794dd6 is unreachable.
    op.at(0x794dc8);const auto a=fp.from_int64(power);op.at(0x794dd0);auto bvalue=fp.from_int64(fraction);
    op.at(0x794ddc);bvalue=fp.multiply_float_memory(bvalue,bits_float(0x37800000));op.at(0x794de9);const auto c=fp.multiply(a,bvalue);
    op.at(0x794deb);const auto k=fp.reverse_divide_float_memory(c,1.0F);
    for(std::uint32_t i=0;i<128;++i){
        const auto w=op.need(o.device.w_at_3d12a00[i],0x794df3,0x3d12a00+4*i);
        op.at(0x794df3);const auto product=fp.multiply_float_memory(k,w);
        op.at(0x794dff);if(fp.compare(fp.from_float(bits_float(0x37800000)),product)==X87Comparison::less){op.at(0x794e0e);o.device.reciprocal_at_3d12c00[i]=fp.to_float(fp.divide(fp.from_float(1.0F),product));}
        else o.device.reciprocal_at_3d12c00[i]=bits_float(0x47800000);
    }
    apply_7949f0(op,o);
}
} // namespace

NinjaFogResult ninja_fog_initialize_794ec0(NinjaFogDeviceOwners& o,const NinjaFogDeviceFacts& facts,X87Context& fp){
    Operation op(&fp);
    return op.run([&]{
        // The 3d12798 store at 794ecc lies between the device load (794ec1) and its dereference (794ed2).
        if(o.render&&o.render->device_at_3d128b0)o.state.fog_enabled_at_3d12798=0;
        op.render_state(o.render,28,0,{0x794ec1,0x794ed2,0x794ed5});
        const auto caps=op.need(facts.raster_caps_from_3d127e4,0x794edb,0x3d127e4);
        o.state.fog_supported_at_3d128a4=(caps>>20)&1U;
        const auto flags=op.need(facts.flags_from_3d08ae0,0x795930,0x3d08ae0);
        bool fallback=true;
        if(flags&0x20U)o.state.fog_supported_at_3d128a4=0;
        else if(*o.state.fog_supported_at_3d128a4!=0){
            fallback=false;
            op.render_state(o.render,fog_color_state,0xffffff,{0x794f92,0x794f97,0x794fa1});
            op.render_state(o.render,fog_table_mode_state,3,{0x794fa7,0x794fac,0x794fb3});
            op.render_state(o.render,fog_start_state,0,{0x794fb9,0x794fbe,0x794fd0});
            op.render_state(o.render,fog_end_state,0x477fff00,{0x794fd6,0x794fdb,0x794fed});
        }
        if(fallback)o.device.software_density_at_3d128a0=0x3f800000;
        o.device.color_at_3d12898=0xffffff;o.device.end_index_at_3d0fce0=0x7e;o.device.start_index_at_3d12950=0x7f;
        // pow(2.0, (i>>4)&15) for i>>4 <= 7 is an exact power of two, so the
        // double products are exact under every CW and store exactly to float.
        for(unsigned i=0;i<128;++i)o.device.w_at_3d12a00[i]=static_cast<float>(std::ldexp(1.0,static_cast<int>((i>>4)&15U))*(static_cast<double>(i&15U)+16.0)*0.0625);
        reciprocal_794db0(op,o,0x8010);
    });
}
NinjaFogResult ninja_fog_color_7949b0(std::uint32_t color,NinjaFogDeviceOwners& o){
    Operation op(nullptr);
    return op.run([&]{
        color&=0xffffffU;
        if(op.need(o.device.color_at_3d12898,0x7949b4,0x3d12898)==color)return;
        const auto supported=op.need(o.state.fog_supported_at_3d128a4,0x7949c3,0x3d128a4);
        o.device.color_at_3d12898=color;
        if(supported)op.render_state(o.render,fog_color_state,color,{0x7949d2,0x7949d8,0x7949de});
    });
}
NinjaFogResult ninja_fog_generate_table_787260(NinjaFogTable& table,float start,float end,NinjaFogDeviceOwners& o,X87Context& fp){
    Operation op(&fp);
    return op.run([&]{
        std::uint32_t p=0x8010;
        op.at(0x787266);if(fp.compare_float_memory(fp.from_float(start),bits_float(0xc77fff00))!=X87Comparison::less){
            op.at(0x78728b);const double negative_end=fp.to_double(fp.from_float(end).negated());
            if(!std::isfinite(negative_end))op.stop(NinjaFogFailure::unmodeled_operation,0x78728e,0x7b5eef); // CRT frexp of Inf/NaN is not modeled
            int exponent=0;const double mantissa=std::frexp(negative_end,&exponent); // exact for finite values
            op.at(0x787293);const auto scaled=fp.multiply_float_memory(fp.from_double(mantissa),65536.0F);
            op.at(0x78729c);const auto truncated=ftol2_644340(fp,scaled);
            const auto low=static_cast<std::int32_t>(static_cast<std::int8_t>(static_cast<std::uint8_t>(exponent-1)));
            p=static_cast<std::uint32_t>(low)|(static_cast<std::uint32_t>(truncated)&0xff00U);
        }
        reciprocal_794db0(op,o,p);
        op.at(0x7872c1);const auto scaled_end=fp.multiply_float_memory(fp.from_float(end),0.00390625F);
        op.at(0x7872ca);const bool beyond=fp.compare_float_memory(scaled_end,start)==X87Comparison::greater;
        const auto x=beyond?fp.from_float(end).negated():fp.from_float(1.0F);
        const auto negative_start=fp.from_float(start).negated();
        std::uint32_t count=0;
        for(std::uint32_t i=0;i<128;++i){
            op.at(0x78730d);const auto power=fp.from_int64(std::int64_t{1}<<(i>>4));const auto low=fp.from_int64(i&15U);
            op.at(0x787319);const auto sum=fp.add_float_memory(low,16.0F);op.at(0x78731f);auto w=fp.multiply(power,sum);
            op.at(0x787321);w=fp.multiply_float_memory(w,0.0625F);op.at(0x787327);const auto q=fp.divide(x,w);
            op.at(0x787329);table[i]=fp.to_float(q);
            op.at(0x78732e);if(fp.compare(negative_start,q)==X87Comparison::less)++count;
        }
        op.at(0x78752d);if(fp.compare_quiet(x,fp.from_float(end).negated())!=X87Comparison::equal)return;
        if(count>=table.size())op.stop(NinjaFogFailure::index_out_of_range,0x78753a); // reads 3b11178, past the table
        op.at(0x78753a);const auto reference=fp.from_float(op.need(table[count],0x78753a,0x3b10f78+4*count));
        const auto normalize=[&](std::uint32_t j,X87Value denominator,std::uint32_t base){
            op.at(base);auto v=fp.add_float_memory(fp.from_float(end),*table[j]);op.at(base+3);v=fp.divide(v,denominator);
            op.at(base+5);v=fp.reverse_subtract_float_memory(v,1.0F);op.at(base+0xb);table[j]=fp.to_float(v);};
        std::uint32_t done=0;
        if(count>=4){
            op.at(0x78754e);const auto denominator=fp.add(fp.from_float(end),reference);
            done=((count-4)>>2)+1;done*=4;
            for(std::uint32_t j=0;j<done;++j)normalize(j,denominator,0x787560+0x12*(j&3));
        }
        if(done<count){
            op.at(0x7875ac);const auto denominator=fp.add_float_memory(reference,end);
            for(std::uint32_t j=done;j<count;++j)normalize(j,denominator,0x7875b4+8);
        }
        for(std::uint32_t j=count;j<table.size();++j)table[j]=0.0F; // REP STOSD of zero words
    });
}
NinjaFogResult ninja_fog_set_table_794e40(const NinjaFogTable& table,NinjaFogDeviceOwners& o,X87Context& fp){
    Operation op(&fp);
    return op.run([&]{
        const auto entry=[&](std::uint32_t i,std::uint32_t source){return op.need(table[i],source,0x3b10f78+4*i);};
        std::uint32_t start=0x7e;
        for(std::uint32_t i=0;i<0x7e;++i)if(float_bits(entry(i,0x794e50))!=0x3f800000){start=i;break;}
        std::uint32_t end=start+1;
        if(start!=0x7e)for(std::uint32_t i=0x7e;i!=start;--i){
            op.at(0x794e70);const auto value=fp.from_float(entry(i,0x794e70));op.at(0x794e79);
            if(fp.compare_quiet(fp.from_float(0.0F),value)!=X87Comparison::equal){end=i+1;break;}
        }
        o.device.end_index_at_3d0fce0=end;o.device.start_index_at_3d12950=start;
        op.at(0x794e98);o.device.end_value_at_3d12894=fp.to_float(fp.from_float(entry(end,0x794e98)));
        op.at(0x794ea1);o.device.start_value_at_3d0fac0=fp.to_float(fp.from_float(entry(start,0x794ea1)));
        apply_7949f0(op,o);
    });
}

NinjaFogResult ninja_fog_411af0_411b40(bool enabled,NinjaFogOwners& o,X87Context& fp){
    Operation op(&fp);auto& state=o.state;
    const auto fog_switch=[&](bool on){const auto prior=op.need(state.fog_enabled_at_3d12798,on?0x794950:0x794980,0x3d12798);
        if((on&&prior==0)||(!on&&prior!=0)){
            const auto supported=op.need(state.fog_supported_at_3d128a4,on?0x794959:0x794989,0x3d128a4);
            state.fog_enabled_at_3d12798=on?1:0;
            if(supported)op.render_state(o.render,28,on?1:0,on?NinjaRenderSourceSites{0x79496c,0x794971,0x794978}:NinjaRenderSourceSites{0x79499c,0x7949a1,0x7949a8});
        }
        state.fog_mode_at_3d0849c=on?1:0; // 787225/787235, after the actual return
    };
    // A nested fog-device result merges into this one; a failure stops here.
    const auto nested=[&](NinjaFogResult n){
        auto& all=op.r.render;all.commands.insert(all.commands.end(),n.render.commands.begin(),n.render.commands.end());
        all.ignored_hresults.insert(all.ignored_hresults.end(),n.render.ignored_hresults.begin(),n.render.ignored_hresults.end());
        if(n.last_fp_site){op.site=*n.last_fp_site;op.r.last_fp_site=n.last_fp_site;}
        if(n)return;
        all.failure=n.render.failure;all.source_boundary=n.render.source_boundary;all.pending_command=n.render.pending_command;
        op.r.required_fact=n.required_fact;op.r.fp_failure=n.fp_failure;op.r.fp_exceptions=n.fp_exceptions;op.stop(n.failure,n.source_boundary,n.callee_boundary);
    };
    // 787240(colour), 787260(table,start,end), 787250(table): the shared update tail.
    const auto update=[&](std::uint32_t color,float start,float end,std::uint32_t color_call){
        if(!o.device||!o.table_at_3b10f78)op.stop(NinjaFogFailure::unlifted_callee,color_call,0x787240);
        NinjaFogDeviceOwners device{state,*o.device,o.render};
        nested(ninja_fog_color_7949b0(color,device));
        nested(ninja_fog_generate_table_787260(*o.table_at_3b10f78,start,end,device,fp));
        nested(ninja_fog_set_table_794e40(*o.table_at_3b10f78,device,fp));
    };
    return op.run([&]{
        const std::uint32_t entry=enabled?0x411af0:0x411b40;
        if(!o.display_switch_at_3b1117c){op.r.required_fact=0x3b1117c;op.stop(NinjaFogFailure::unknown_fact,entry);}
        if(op.need(*o.display_switch_at_3b1117c,entry,0x3b1117c)!=0)return;
        if(!enabled){o.flag_at_3abdcfe=0;fog_switch(false);return;}
        const auto profile=op.need(state.fog_profile_at_3abdc6c,0x411af9,0x3abdc6c);
        if(profile==0){
            op.at(0x4119b0);auto sentinel=fp.from_float(o.fog_sentinel_at_7dce50);
            const auto start=op.need(state.cached_fog_start_at_90a098,0x4119b6,0x90a098);op.at(0x4119b6);bool changed=fp.compare_float_memory(sentinel,start)!=X87Comparison::equal;
            if(!changed){op.at(0x4119c3);sentinel=fp.from_float(o.fog_sentinel_at_7dce50);const auto end=op.need(state.cached_fog_end_at_90a09c,0x4119c9,0x90a09c);op.at(0x4119c9);changed=fp.compare_float_memory(sentinel,end)!=X87Comparison::equal;}
            if(changed){
                update(0,bits_float(0xc77fff00),bits_float(0xc77fff00),0x4119d8);
                state.cached_fog_start_at_90a098=bits_float(0xc77fff00);state.cached_fog_end_at_90a09c=bits_float(0xc77fff00);
                fog_switch(false); // 411a12 JMP 787230, returning into 411b33's own JMP 787230
            }
            fog_switch(false);return;
        }
        const auto special=op.need(state.special_flags_at_909eb4,0x411a20,0x909eb4);
        if(!(special&1)){
            // Actual 411a20 level fog: both range words are negated when positive.
            if(!o.level_start_at_3abdc60||!o.level_end_at_3abdc64||!o.level_color_at_3abdc68)op.stop(NinjaFogFailure::unlifted_callee,0x411b02,0x411a20);
            float start=op.need(*o.level_start_at_3abdc60,0x411a30,0x3abdc60),end=op.need(*o.level_end_at_3abdc64,0x411a35,0x3abdc64);
            const auto color=op.need(*o.level_color_at_3abdc68,0x411a3b,0x3abdc68);
            op.at(0x411a4d);if(fp.compare_float_memory(fp.from_float(start),0.0F)==X87Comparison::greater){op.at(0x411a60);start=fp.to_float(fp.from_float(start).negated());}
            op.at(0x411a68);if(fp.compare_float_memory(fp.from_float(end),0.0F)==X87Comparison::greater){op.at(0x411a7b);end=fp.to_float(fp.from_float(end).negated());}
            if(!o.device||!o.table_at_3b10f78)op.stop(NinjaFogFailure::unlifted_callee,0x411a80,0x787240);
            NinjaFogDeviceOwners device{state,*o.device,o.render};
            nested(ninja_fog_color_7949b0(color,device));
            const auto cached_start=op.need(state.cached_fog_start_at_90a098,0x411a89,0x90a098);
            op.at(0x411a89);bool changed=fp.compare_float_memory(fp.from_float(start),cached_start)!=X87Comparison::equal;
            if(!changed){const auto cached_end=op.need(state.cached_fog_end_at_90a09c,0x411a9d,0x90a09c);op.at(0x411a9d);changed=fp.compare_float_memory(fp.from_float(end),cached_end)!=X87Comparison::equal;}
            if(changed){nested(ninja_fog_generate_table_787260(*o.table_at_3b10f78,start,end,device,fp));nested(ninja_fog_set_table_794e40(*o.table_at_3b10f78,device,fp));}
            state.cached_fog_start_at_90a098=start;state.cached_fog_end_at_90a09c=end;
            fog_switch(true);o.flag_at_3abdcfe=1; // 411b14 CALL 787220
            return;
        }
        fog_switch(false);o.flag_at_3abdcfe=1; // 411a20 returned 1 at once; 411b21 CALL 787230
    });
}
} // namespace sadx
