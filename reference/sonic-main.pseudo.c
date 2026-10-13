    eax = arg_0;
    decomp_var_1c = ebx;
    ebx = eax[0x8];
    decomp_var_20 = ebp;
    esp = esp - 0x24;
    decomp_var_24 = esi;
    esi = eax[0x9];
    ebp = *esi;
    *dword_3abdf60 = ebx;
    *dword_3abdf64 = ebp;
    ecx = ebx[0x2];
    var_18 = esi;
    if ((ecx & 0x20) != 0x0) {
            asm { fld        dword [esi+8] };
            asm { fcomp      dword [dword_value_1142947840+4] };
            asm { fnstsw     ax };
            if (!PARITY(HIBYTE(eax) & 0x41)) {
                    eax = ebx[0xe];
                    ebx[0x2] = ecx | 0x2;
                    ebx[0x5] = *(*(eax[0x27] + 0x20) + 0x14);
                    ebx[0x7] = *(*(eax[0x27] + 0x20) + 0x1c);
            }
            else {
                    ebx[0x2] = ecx & 0xfffffffd;
            }
    }
    else {
            ebx[0x2] = ecx & 0xfffffffd;
    }
    if (sub_4258f0() != 0x0) {
            *(int8_t *)ebx = 0x57;
    }
    else {
            if (*(int8_t *)ebx != 0x56) {
                    sub_496f50(ebx, esi, ebp);
                    esp = (esp - 0xc) + 0xc;
            }
    }
    eax = ebx[0xe];
    bool_88 = eax == 0x0;
    bool_4 = eax == 0x0;
    esp = esp - 0x4;
    var_28 = edi;
    if (eax != 0x0) {
            edi = eax[0x3];
            if (!bool_4) {
                    ecx = 0x3;
                    eax = 0x3;
            }
            else {
                    if (!bool_88) {
                            ecx = 0x3;
                            eax = 0x3;
                    }
                    else {
                            eax = *(int8_t *)ebx;
                            if (eax == 0x2c) {
                                    ecx = 0x2;
                                    eax = 0x3;
                            }
                            else {
                                    if (eax == 0x6) {
                                            ecx = 0x1;
                                            eax = 0x3;
                                    }
                                    else {
                                            if ((ebx[0x2] & 0x100) != 0x0) {
                                                    ecx = 0x1;
                                                    eax = 0x1;
                                            }
                                            else {
                                                    ecx = 0x0;
                                                    if (eax == 0x2d) {
                                                            eax = 0x1;
                                                    }
                                                    else {
                                                            eax = 0x0;
                                                    }
                                            }
                                    }
                            }
                    }
            }
            edi[0x3] = (eax & 0x3) << 0x2 | edi[0x3] & 0xf0 | ecx & 0x3;
            if (*(int8_t *)ebx == 0x48) {
                    eax = ebx[0xe];
                    eax = eax[0x3];
                    ecx = eax[0xd];
                    ecx = ecx & 0xffffffef;
            }
            else {
                    eax = *(ebx[0xe] + 0xc);
                    ecx = eax[0xd];
                    ecx = ecx | 0x10;
            }
            eax[0xd] = ecx;
    }
    eax = *(int8_t *)ebx;
    if (eax >= 0x4b) {
            *(int8_t *)byte_3c55d45 = 0x1;
            if (eax > 0x52) {
                    *(int8_t *)byte_3c55d45 = 0x0;
            }
    }
    else {
            *(int8_t *)byte_3c55d45 = 0x0;
    }
    eax = sign_extend_32(*(int8_t *)ebx);
    if (eax <= 0x57) {
            switch (*(eax * 0x4 + switch_table)) {
                case 0:
                    ecx = ebx[0x8] & 0xff;
                    ebx[0x9] = 0x0;
                    sub_442750(ecx, arg_0);
                    ebp = *esi;
                    arg_120 = 0x0;
                    esp = (esp - 0x8) + 0x8;
                    if (*(int8_t *)byte_3b18db5 == 0x0) {
                            ecx = *dword_38f6e80;
                            eax = ecx[0x6];
                            ecx = ecx[0x5];
                            if (eax[0xc] != ecx) {
                                    eax[0xc] = ecx;
                                    *dword_3c55f38 = *(*(*dword_38f6e80 + 0x18) + 0x30);
                                    sub_43fa90(ebx, esi, ebp);
                                    esp = (esp - 0xc) + 0xc;
                            }
                    }
                    *dword_3c55d40 = *(*(*dword_38f6e80 + 0x18) + 0x30);
                    sub_41caf0(arg_0, 0x91cbf0, 0x2, 0x0);
                    *(*(ebx[0xe] + 0xc) + 0x14) = arg_108;
                    *(int8_t *)ebx = 0x1;
                    sub_421280(0x0);
                    esp = (esp - 0x14) + 0x14;
                    if (*(int8_t *)byte_3b18db5 != 0x0) {
                            sub_421280(0x8);
                            esp = (esp - 0x4) + 0x4;
                    }
                    sub_495860(ebx[0x8]);
                    eax = sub_44a800(&arg_11c);
                    esp = (esp - 0x8) + 0x8;
                    eax = dword_3c55e28;
                    if (*(int8_t *)byte_3b18db5 != 0x0) {
                            eax = dword_3c55d68;
                    }
                    bool_83 = ((sign_extend_32(*(int16_t *)word_3b22dcc) << 0x8 | *(int16_t *)byte_3b22dec) & 0xff00) != 0x1300;
                    ecx = *eax;
                    if (!bool_83) {
                            if (ecx != 0x0) {
                                    do {
                                            if (*eax == *(*dword_38f6e80 + 0x58)) {
                                                    eax[0xd] = 0x2;
                                            }
                                            edi = eax[0x6];
                                            eax = eax + 0x18;
                                    } while (edi != 0x0);
                            }
                    }
                    else {
                            if (ecx != 0x0) {
                                    do {
                                            if (*eax == *(*dword_38f6e80 + 0x58)) {
                                                    eax[0xd] = 0x0;
                                            }
                                            edi = eax[0x6];
                                            eax = eax + 0x18;
                                    } while (edi != 0x0);
                            }
                    }
                    sub_43fa90(ebx, esi, ebp);
                    esp = (esp - 0xc) + 0xc;
                    if (*(int8_t *)byte_3b18db5 == 0x0) {
                            eax = sub_40b860(0x2, 0x3, sub_493730);
                            esp = (esp - 0xc) + 0xc;
                            if (eax != 0x0) {
                                    *(int8_t *)(eax[0x8] + 0x8) = ebx[0x8];
                            }
                            eax = sub_40b860(0x3, 0x3, sub_4937b0);
                            esp = (esp - 0xc) + 0xc;
                            if (eax != 0x0) {
                                    *(int8_t *)(eax[0x8] + 0x8) = ebx[0x8];
                            }
                    }
                    arg_0[0x6] = sub_494860;
                    arg_0[0x5] = sub_4948c0;
                    break;
                case 1:
                    sub_44bb60(ebx, var_18, ebp);
                    sub_44c270(ebx, var_18, ebp);
                    sub_443f50(ebx, var_18, ebp);
                    sub_44cdf0(ebx, var_18, ebp);
                    sub_43ee70(ebx, var_18, ebp);
                    sub_491660();
                    esp = (esp - 0x40) + 0x40;
                    esi = var_18;
                    break;
                case 2:
                    if (arg_124 == 0x1e && arg_122 != 0x1e) {
                            asm { fld        dword [ebp+0xc0] };
                            asm { fadd       dword [ebp+0x38] };
                            asm { fstp       dword [ebp+0x38] };
                    }
                    sub_44c270(ebx, esi, ebp);
                    sub_443f50(ebx, esi, ebp);
                    esp = (esp - 0x24) + 0x24;
                    if (sub_44cdf0(ebx, esi, ebp) == 0x2) {
                            if (ebx[0x8] == 0x0 && *(int8_t *)byte_3b18db5 == 0x0) {
                                    sub_423d70(0x4c6, 0x0, 0x0, 0x0);
                                    esp = (esp - 0x10) + 0x10;
                            }
                            *(int8_t *)ebx = 0x12;
                            arg_120 = 0x1b;
                            sub_440790(ebx);
                            esp = (esp - 0x4) + 0x4;
                            asm { fld        dword [ebp+0x38] };
                            asm { fmul       dword [dword_value_3197737370] };
                            asm { fstp       dword [ebp+0x38] };
                            asm { fld        dword [ebp+0x40] };
                            asm { fmul       dword [dword_value_3197737370] };
                            asm { fstp       dword [ebp+0x40] };
                    }
                    else {
                            sub_43ee70(ebx, esi, ebp);
                            esp = (esp - 0xc) + 0xc;
                            sub_495cd0();
                    }
                    break;
                case 3:
                    sub_442a50(ebx, esi, ebp);
                    sub_4441b0(ebx, esi, ebp);
                    sub_43e100(ebx, esi, ebp);
                    esp = (esp - 0x24) + 0x24;
                    sub_495cd0();
                    break;
                case 4:
                    sub_44bb60(ebx, esi, ebp);
                    sub_448e50(ebx, esi, ebp);
                    sub_443f50(ebx, esi, ebp);
                    sub_44cdf0(ebx, esi, ebp);
                    sub_43ee70(ebx, esi, ebp);
                    eax = sub_40b860(0x2, 0x5, sub_4a2a10);
                    esp = (esp - 0x48) + 0x48;
                    if (eax != 0x0) {
                            *(int8_t *)(eax[0x8] + 0x8) = ebx[0x8];
                    }
                    if ((arg_0 & 0x2) != 0x0) {
                            eax = sub_6443bf();
                            asm { fild       dword [esp+0x28+var_18] };
                            asm { fmul       dword [dword_value_939524096] };
                            asm { fcomp      dword [qword_value_4604029899060858061+8] };
                            asm { fnstsw     ax };
                            if (!PARITY(HIBYTE(eax) & 0x5)) {
                                    edi = 0x3;
                                    do {
                                            sub_4a2040(ebx, 0x1f);
                                            esp = (esp - 0x8) + 0x8;
                                            edi = edi - 0x1;
                                    } while (edi != 0x0);
                            }
                    }
                    break;
                case 5:
                    esp = esp - 0xc;
                    sub_44bb60(ebx, esi, ebp);
                    sub_443650(ebx, esi, ebp);
                    sub_443f50(ebx, esi, ebp);
                    sub_44cdf0(ebx, esi, ebp);
                    esp = esp - 0x30;
                    eax = sub_43ee70(ebx, esi, ebp);
                    asm { fld        dword [ebp+0x38] };
                    asm { fcomp      dword [ebp+0xc4] };
                    esp = esp + 0x3c;
                    asm { fnstsw     ax };
                    if ((HIBYTE(eax) & 0x1) == 0x0) {
                            sub_494050(ebx);
                            arg_78 = arg_78 | 0x1;
                            sub_4940b0(ebx);
                            esp = (esp - 0x8) + 0x8;
                    }
                    break;
                case 6:
                    sub_492ab0(ebx, esi, ebp);
                    esp = (esp - 0x18) + 0x18;
                    if (sub_44cdf0(ebx, esi, ebp) == 0x2) {
                            if (*(int8_t *)byte_3b18db5 == 0x0) {
                                    sub_423d70(0x4c6, 0x0, 0x0, 0x0);
                                    esp = (esp - 0x10) + 0x10;
                            }
                            *(int8_t *)ebx = 0x12;
                            arg_120 = 0x1b;
                            ebx[0x5] = ebx[0x5] & 0xfb;
                            sub_424210(0x2fc);
                            sub_440790(ebx);
                            esp = (esp - 0x8) + 0x8;
                    }
                    else {
                            sub_43ee70(ebx, esi, ebp);
                            sub_494050(ebx);
                            esp = (esp - 0x10) + 0x10;
                            arg_78 = arg_78 | 0x1;
                    }
                    break;
                case 7:
                    sub_443650(ebx, esi, ebp);
                    sub_4441b0(ebx, esi, ebp);
                    sub_43e100(ebx, esi, ebp);
                    sub_4940b0(ebx);
                    esp = (esp - 0x28) + 0x28;
                    break;
                case 8:
                    sub_443ad0(ebx, esi, ebp);
                    esp = (esp - 0x18) + 0x18;
                    if (sub_44bd70(ebx, esi, ebp) == 0x0) {
                            var_2C = ebp;
                            decomp_var_30 = esi;
                            esp = esp - 0xc;
                            var_34 = ebx;
                            sub_44b9c0(var_34, decomp_var_30, var_2C);
                            esp = esp + 0xc;
                    }
                    esp = esp - 0xc;
                    sub_443f50(ebx, esi, ebp);
                    sub_44cdf0(ebx, esi, ebp);
                    sub_43ee70(ebx, esi, ebp);
                    esp = (esp - 0x18) + 0x24;
                    break;
                case 9:
                    esp = esp - 0xc;
                    sub_44b9c0(ebx, esi, ebp);
                    esp = esp - 0xc;
                    sub_443f50(ebx, esi, ebp);
                    sub_44cdf0(ebx, esi, ebp);
                    sub_43ee70(ebx, esi, ebp);
                    esp = (esp - 0x18) + 0x30;
                    break;
                case 10:
                    asm { fld        dword [ebp+0x3c] };
                    esp = esp - 0x4;
                    var_2C = ebp;
                    asm { fcomp      dword [ebp+0xb8] };
                    decomp_var_30 = esi;
                    esp = esp - 0x8;
                    var_34 = ebx;
                    asm { fnstsw     ax };
                    if ((HIBYTE(eax) & 0x1) == 0x0) {
                            sub_4432e0(var_34, decomp_var_30, var_2C);
                            esp = esp + 0xc;
                    }
                    else {
                            sub_44b9c0(var_34, decomp_var_30, var_2C);
                            esp = esp + 0xc;
                    }
                    esp = esp - 0xc;
                    sub_443f50(ebx, esi, ebp);
                    sub_44cdf0(ebx, esi, ebp);
                    sub_43ee70(ebx, esi, ebp);
                    esp = (esp - 0x18) + 0x24;
                    break;
                case 11:
                    esp = esp - 0xc;
                    sub_443800(ebx, esi, ebp);
                    esp = esp - 0xc;
                    sub_443f50(ebx, esi, ebp);
                    sub_44cdf0(ebx, esi, ebp);
                    sub_43ee70(ebx, esi, ebp);
                    esp = (esp - 0x18) + 0x30;
                    break;
                case 12:
                    esp = esp - 0xc;
                    sub_448e50(ebx, esi, ebp);
                    esp = esp - 0xc;
                    sub_443f50(ebx, esi, ebp);
                    sub_44cdf0(ebx, esi, ebp);
                    sub_43ee70(ebx, esi, ebp);
                    esp = (esp - 0x18) + 0x30;
                    break;
                case 13:
                    sub_443ad0(ebx, var_18, ebp);
                    sub_492300(var_18);
                    sub_44cdf0(ebx, var_18, ebp);
                    sub_43ee70(ebx, var_18, ebp);
                    sub_494050(ebx);
                    arg_78 = arg_78 | 0x1;
                    sub_4940b0(ebx);
                    esp = (esp - 0x30) + 0x30;
                    esi = var_18;
                    break;
                case 14:
                    var_2C = ebp;
                    decomp_var_30 = esi;
                    esp = esp - 0xc;
                    var_34 = ebx;
                    if (arg_120 == 0x15) {
                            sub_448e50(var_34, decomp_var_30, var_2C);
                            esp = esp + 0xc - 0xc;
                            sub_4442c0(ebx, esi, ebp);
                    }
                    else {
                            sub_442b50(var_34, decomp_var_30, var_2C);
                            esp = esp + 0xc;
                            if (*(int8_t *)byte_3b18db5 == 0x0) {
                                    esp = (esp - 0x4) + 0x4;
                                    if ((HIBYTE(sub_43b9c0(ebx)) & 0x4) != 0x0) {
                                            sub_423f50(0xf, ebx, 0x1, 0x0, 0x2);
                                            esp = (esp - 0x14) + 0x14;
                                    }
                                    if (arg_124 == 0x15) {
                                            sub_423d70(0x4ca, 0x0, 0x0, 0x0);
                                            esp = (esp - 0x10) + 0x10;
                                    }
                            }
                            esp = esp - 0xc;
                            sub_4442c0(ebx, esi, ebp);
                    }
                    sub_44cdf0(ebx, esi, ebp);
                    sub_43ee70(ebx, esi, ebp);
                    esp = (esp - 0x18) + 0x24;
                    break;
                case 15:
                    if ((ebx[0x4] & 0x3) != 0x0) {
                            sub_443ad0(ebx, esi, ebp);
                            esp = (esp - 0xc) + 0xc;
                    }
                    esp = esp - 0xc;
                    sub_448e50(ebx, esi, ebp);
                    esp = esp - 0xc;
                    sub_443f50(ebx, esi, ebp);
                    sub_44cdf0(ebx, esi, ebp);
                    sub_43ee70(ebx, esi, ebp);
                    esp = (esp - 0x18) + 0x30;
                    break;
                case 16:
                    sub_44bb60(ebx, esi, ebp);
                    esp = esp - 0x18;
                    sub_4432e0(ebx, esi, ebp);
                    sub_443f50(ebx, esi, ebp);
                    sub_44cdf0(ebx, esi, ebp);
                    sub_43ee70(ebx, esi, ebp);
                    esp = (esp - 0x24) + 0x3c;
                    break;
                default:
                    if (*(eax * 0x4 + switch_table) != 0x11) {
                            switch (*(eax * 0x4 + switch_table)) {
                                case 18:
                                    esp = esp - 0xc;
                                    sub_4432e0(ebx, esi, ebp);
                                    esp = esp - 0xc;
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x18) + 0x30;
                                    break;
                                case 19:
                                    sub_44bb60(ebx, esi, ebp);
                                    esp = esp - 0x18;
                                    sub_44c270(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x24) + 0x3c;
                                    break;
                                case 20:
                                    sub_442b50(ebx, esi, ebp);
                                    esp = esp - 0x18;
                                    sub_4442c0(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x18) + 0x30;
                                    break;
                                case 21:
                                    if (arg_124 == 0x24 && arg_122 != 0x24) {
                                            asm { fld        dword [ebp+0xc0] };
                                            asm { fadd       dword [ebp+0x38] };
                                            asm { fstp       dword [ebp+0x38] };
                                    }
                                    sub_44c270(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x30) + 0x30;
                                    sub_495cd0();
                                    break;
                                case 22:
                                    sub_44c1c0(ebx, esi, ebp);
                                    esp = esp - 0x18;
                                    sub_4432e0(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x24) + 0x3c;
                                    break;
                                case 23:
                                    sub_443ad0(ebx, esi, ebp);
                                    esp = (esp - 0x18) + 0x18;
                                    if (sub_44bd70(ebx, esi, ebp) == 0x0) {
                                            sub_44b9c0(ebx, esi, ebp);
                                            esp = (esp - 0xc) + 0xc;
                                    }
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x24) + 0x24;
                                    break;
                                case 24:
                                    sub_442b50(ebx, esi, ebp);
                                    sub_4494f0(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x30) + 0x30;
                                    if (*(int8_t *)byte_3b18db5 == 0x0) {
                                            esp = (esp - 0x4) + 0x4;
                                            if ((HIBYTE(sub_43b9c0(ebx)) & 0x4) != 0x0) {
                                                    sub_423f50(0xf, ebx, 0x1, 0x0, 0x2);
                                                    esp = (esp - 0x14) + 0x14;
                                            }
                                    }
                                    break;
                                case 25:
                                    sub_443ad0(ebx, esi, ebp);
                                    sub_44c1c0(ebx, esi, ebp);
                                    sub_4432e0(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x48) + 0x48;
                                    break;
                                case 26:
                                    sub_43bb60(ebx, esi, ebp);
                                    esp = (esp - 0xc) + 0xc;
                                    break;
                                case 27:
                                    asm { fld        dword [ebp+0x130] };
                                    edx = arg_64[0x9];
                                    asm { fmul       dword [dword_value_1008981770] };
                                    asm { fstp       dword [edx+0x18] };
                                    eax = arg_64[0x9];
                                    eax[0x5] = eax[0x6];
                                    eax = arg_64[0x9];
                                    eax[0x4] = eax[0x5];
                                    break;
                                case 28:
                                    sub_43e2f0(ebx, var_18, ebp);
                                    sub_4918c0(var_18);
                                    esp = (esp - 0x10) + 0x10;
                                    esi = var_18;
                                    break;
                                case 29:
                                    sub_43e510(ebx, var_18, ebp);
                                    sub_491b40(var_18);
                                    esp = (esp - 0x10) + 0x10;
                                    esi = var_18;
                                    break;
                                case 30:
                                    esp = (esp - 0xc) + 0xc;
                                    if (sub_4b8910(ebx, esi, ebp) <= 0x1f) {
                                            arg_120 = 0x48;
                                    }
                                    break;
                                case 31:
                                    sub_440740(0x0, 0x1e);
                                    sub_4432e0(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    eax = sub_43bdc0(ebx, &var_10, &var_18);
                                    esp = (esp - 0x2c) + 0x2c;
                                    if (eax != 0x0) {
                                            asm { fild       dword [esp+0x28+var_14] };
                                            asm { fmul       dword [dword_value_939524096] };
                                            asm { fsubr      dword [dword_value_1056964608] };
                                            asm { fmul       dword [esp+0x28+var_18] };
                                            asm { fstp       dword [esi+4] };
                                            if ((ebx[0x4] & 0x3) != 0x0) {
                                                    eax = sub_6443bf();
                                                    asm { fild       dword [esp+0x28+var_14] };
                                                    asm { fmul       dword [dword_value_939524096] };
                                                    asm { fcomp      dword [dword_value_1114767360+4] };
                                                    asm { fnstsw     ax };
                                                    if ((HIBYTE(eax) & 0x41) == 0x0) {
                                                            asm { fild       dword [esp+0x28+var_14] };
                                                            asm { fmul       dword [dword_value_939524096] };
                                                            asm { fmul       dword [dword_value_1055286886+4] };
                                                            asm { fadd       dword [dword_value_1050253722] };
                                                            asm { fstp       dword [esi+8] };
                                                    }
                                            }
                                            asm { fild       dword [esp+0x28+var_14] };
                                            asm { fmul       dword [dword_value_939524096] };
                                            asm { fsubr      dword [dword_value_1056964608] };
                                            asm { fmul       dword [esp+0x28+var_18] };
                                            asm { fstp       dword [esi+0xc] };
                                    }
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x18) + 0x18;
                                    break;
                                case 32:
                                    sub_443ad0(ebx, esi, ebp);
                                    esp = (esp - 0x10) + 0x10;
                                    if (sub_4927d0(esi) == 0x0) {
                                            *(int8_t *)ebx = 0xc;
                                            arg_120 = 0x12;
                                            ebx[0x5] = ebx[0x5] & 0xfa;
                                    }
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    sub_494050(ebx);
                                    esp = (esp - 0x1c) + 0x1c;
                                    arg_78 = arg_78 | 0x1;
                                    break;
                                case 33:
                                    sub_448150(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    esp = (esp - 0x24) + 0x24;
                                    if (sub_43e610(ebx, esi, ebp) == 0x2) {
                                            if (*(int8_t *)byte_3b18db5 == 0x0) {
                                                    sub_423d70(0x4c6, 0x0, 0x0, 0x0);
                                                    esp = (esp - 0x10) + 0x10;
                                            }
                                            *(int8_t *)ebx = 0x32;
                                            esp = esp - 0x4;
                                            arg_120 = 0x2e;
                                            sub_440790(ebx);
                                            asm { fld        dword [edx+0x1c] };
                                            esp = esp + 0x4;
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            asm { fadd       dword [ebx+0x20] };
                                            asm { fstp       dword [ebx+0x20] };
                                            asm { fld        dword [eax+0x20] };
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            asm { fadd       dword [ebx+0x24] };
                                            asm { fstp       dword [ebx+0x24] };
                                            asm { fld        dword [ecx+0x24] };
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            asm { fadd       dword [ebx+0x28] };
                                            asm { fstp       dword [ebx+0x28] };
                                            asm { fld        dword [ebp+0x38] };
                                            asm { fmul       dword [dword_value_1055286886+4] };
                                            asm { fstp       dword [ebp+0x38] };
                                            ebx[0x5] = 0x0;
                                            ebx[0x7] = 0xc000;
                                    }
                                    else {
                                            sub_43f010(ebx, esi, ebp);
                                            esp = (esp - 0xc) + 0xc;
                                            sub_495cd0();
                                    }
                                    break;
                                case 34:
                                    sub_448150(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    esp = (esp - 0x24) + 0x24;
                                    if (sub_43e610(ebx, esi, ebp) == 0x2) {
                                            if (*(int8_t *)byte_3b18db5 == 0x0) {
                                                    sub_423d70(0x4c6, 0x0, 0x0, 0x0);
                                                    esp = (esp - 0x10) + 0x10;
                                            }
                                            ebx[0x5] = ebx[0x5] & 0xfa;
                                            *(int8_t *)ebx = 0x32;
                                            esp = esp - 0x4;
                                            arg_120 = 0x2e;
                                            sub_440790(ebx);
                                            asm { fld        dword [eax+0x1c] };
                                            esp = esp + 0x4;
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            asm { fadd       dword [ebx+0x20] };
                                            asm { fstp       dword [ebx+0x20] };
                                            asm { fld        dword [ecx+0x20] };
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            asm { fadd       dword [ebx+0x24] };
                                            asm { fstp       dword [ebx+0x24] };
                                            asm { fld        dword [edx+0x24] };
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            asm { fadd       dword [ebx+0x28] };
                                            asm { fstp       dword [ebx+0x28] };
                                            eax = arg_b0;
                                            asm { fld        dword [ebp+0x38] };
                                            asm { fmul       dword [dword_value_1055286886+4] };
                                            asm { fstp       dword [ebp+0x38] };
                                            ebx[0x5] = 0x0;
                                            ebx[0x7] = 0xc000;
                                    }
                                    else {
                                            esp = esp - 0xc;
                                            eax = sub_43f010(ebx, esi, ebp);
                                            esp = esp + 0xc;
                                    }
                                    asm { fld        dword [ebp+0x38] };
                                    asm { fcomp      dword [ebp+0xc4] };
                                    asm { fnstsw     ax };
                                    if ((HIBYTE(eax) & 0x1) == 0x0) {
                                            sub_494050(ebx);
                                            arg_78 = arg_78 | 0x1;
                                            sub_4940b0(ebx);
                                            esp = (esp - 0x8) + 0x8;
                                    }
                                    break;
                                case 35:
                                    sub_443b90(ebx, esi, ebp);
                                    sub_448150(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    esp = (esp - 0x30) + 0x30;
                                    if (sub_43e610(ebx, esi, ebp) == 0x2) {
                                            if (*(int8_t *)byte_3b18db5 == 0x0) {
                                                    sub_423d70(0x4c6, 0x0, 0x0, 0x0);
                                                    esp = (esp - 0x10) + 0x10;
                                            }
                                            *(int8_t *)ebx = 0x32;
                                            esp = esp - 0x4;
                                            arg_120 = 0x2e;
                                            sub_440790(ebx);
                                            asm { fld        dword [edx+0x1c] };
                                            esp = esp + 0x4;
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            var_2C = ebp;
                                            decomp_var_30 = esi;
                                            esp = esp - 0xc;
                                            var_34 = ebx;
                                            asm { fadd       dword [ebx+0x20] };
                                            asm { fstp       dword [ebx+0x20] };
                                            asm { fld        dword [eax+0x20] };
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            asm { fadd       dword [ebx+0x24] };
                                            asm { fstp       dword [ebx+0x24] };
                                            asm { fld        dword [ecx+0x24] };
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            asm { fadd       dword [ebx+0x28] };
                                            asm { fstp       dword [ebx+0x28] };
                                            asm { fld        dword [ebp+0x38] };
                                            asm { fmul       dword [dword_value_1055286886+4] };
                                            asm { fstp       dword [ebp+0x38] };
                                            ebx[0x5] = 0x0;
                                            ebx[0x7] = 0xc000;
                                            sub_43f010(var_34, decomp_var_30, var_2C);
                                            esp = esp + 0xc;
                                    }
                                    else {
                                            sub_43f010(ebx, esi, ebp);
                                            sub_43f010(ebx, esi, ebp);
                                            esp = ((esp - 0xc) + 0xc - 0xc) + 0xc;
                                    }
                                    break;
                                case 36:
                                    sub_443b90(ebx, esi, ebp);
                                    sub_448150(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    esp = (esp - 0x30) + 0x30;
                                    if (sub_43e610(ebx, esi, ebp) == 0x2) {
                                            if (*(int8_t *)byte_3b18db5 == 0x0) {
                                                    sub_423d70(0x4c6, 0x0, 0x0, 0x0);
                                                    esp = (esp - 0x10) + 0x10;
                                            }
                                            *(int8_t *)ebx = 0x32;
                                            esp = esp - 0x4;
                                            arg_120 = 0x2e;
                                            sub_440790(ebx);
                                            asm { fld        dword [ecx+0x1c] };
                                            esp = esp + 0x4;
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            var_2C = ebp;
                                            decomp_var_30 = esi;
                                            esp = esp - 0xc;
                                            var_34 = ebx;
                                            asm { fadd       dword [ebx+0x20] };
                                            asm { fstp       dword [ebx+0x20] };
                                            asm { fld        dword [edx+0x20] };
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            asm { fadd       dword [ebx+0x24] };
                                            asm { fstp       dword [ebx+0x24] };
                                            asm { fld        dword [eax+0x24] };
                                            asm { fmul       dword [dword_value_1074301065+4] };
                                            asm { fadd       dword [ebx+0x28] };
                                            asm { fstp       dword [ebx+0x28] };
                                            asm { fld        dword [ebp+0x38] };
                                            asm { fmul       dword [dword_value_1055286886+4] };
                                            asm { fstp       dword [ebp+0x38] };
                                            ebx[0x5] = ebx[0x5] & 0xfa;
                                            ebx[0x5] = 0x0;
                                            ebx[0x7] = 0xc000;
                                            sub_43f010(var_34, decomp_var_30, var_2C);
                                            esp = esp + 0xc;
                                    }
                                    else {
                                            sub_43f010(ebx, esi, ebp);
                                            sub_43f010(ebx, esi, ebp);
                                            esp = ((esp - 0xc) + 0xc - 0xc) + 0xc;
                                    }
                                    break;
                                case 37:
                                    sub_448150(ebx, esi, ebp);
                                    esp = esp - 0x18;
                                    sub_443f50(ebx, esi, ebp);
                                    asm { fld        dword [esi+8] };
                                    asm { fadd       dword [ebx+0x24] };
                                    decomp_var_44 = ebp;
                                    decomp_var_48 = esi;
                                    esp = esp - 0xc;
                                    decomp_var_4c = ebx;
                                    asm { fstp       dword [ebx+0x24] };
                                    sub_43f010(decomp_var_4c, decomp_var_48, decomp_var_44);
                                    esp = esp + 0x24;
                                    break;
                                case 38:
                                    sub_493460();
                                    esp = (esp - 0x4) + 0x4;
                                    break;
                                case 39:
                                    asm { fld        dword [ebx+0x24] };
                                    eax = arg_64;
                                    asm { fsub       dword [dword_value_1056964608] };
                                    eax = eax[0x8];
                                    asm { fst        dword [ebx+0x24] };
                                    ecx = eax[0xe];
                                    ecx = ecx[0x3];
                                    asm { fld        dword [ecx+0xc] };
                                    asm { fadd       dword [eax+0x24] };
                                    asm { fsub       dword [ecx+0x18] };
                                    asm { fst        dword [esp+0x28+var_14] };
                                    asm { fcomp      st1 };
                                    asm { fnstsw     ax };
                                    asm { fstp       st0 };
                                    if ((HIBYTE(eax) & 0x1) == 0x0) {
                                            ebx[0x9] = var_14;
                                    }
                                    sub_493460();
                                    esp = (esp - 0x4) + 0x4;
                                    break;
                                case 40:
                                    esp = esp - 0x8;
                                    eax = sub_45ab70();
                                    edx = ebx[0x6];
                                    edx = edx + eax;
                                    ebx[0x6] = edx;
                                    sub_493460();
                                    esp = (esp - 0x4) + 0xc;
                                    break;
                                case 41:
                                    esp = esp - 0x8;
                                    eax = sub_45ab70();
                                    edx = ebx[0x6];
                                    edx = edx - eax;
                                    ebx[0x6] = edx;
                                    sub_493460();
                                    esp = (esp - 0x4) + 0xc;
                                    break;
                                case 42:
                                    sub_4934b0(var_18);
                                    esp = (esp - 0x4) + 0x4;
                                    esi = var_18;
                                    break;
                                case 43:
                                    sub_4934b0(var_18);
                                    esp = (esp - 0x4) + 0x4;
                                    esi = var_18;
                                    break;
                                case 44:
                                    ebx[0x6] = ebx[0x6] + 0x300 & 0xfe00;
                                    sub_4934b0(var_18);
                                    esp = (esp - 0x4) + 0x4;
                                    esi = var_18;
                                    break;
                                case 45:
                                    ebx[0x6] = ebx[0x6] - 0x300 & 0xfe00;
                                    sub_4934b0(var_18);
                                    esp = (esp - 0x4) + 0x4;
                                    esi = var_18;
                                    break;
                                case 46:
                                    asm { fld        dword [ebx+0x24] };
                                    asm { fadd       dword [dword_value_1050253722] };
                                    asm { fstp       dword [ebx+0x24] };
                                    sub_4934b0(var_18);
                                    esp = (esp - 0x4) + 0x4;
                                    esi = var_18;
                                    break;
                                case 47:
                                    sub_4348a0(0x1);
                                    sub_443ad0(ebx, esi, ebp);
                                    sub_443650(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x40) + 0x40;
                                    break;
                                case 48:
                                    sub_448550(ebx, esi, ebp, 0x40bf5c29);
                                    sub_443f50(ebx, esi, ebp);
                                    esp = (esp - 0x28) + 0x28;
                                    if (sub_44cdf0(ebx, esi, ebp) == 0x2) {
                                            if (ebx[0x8] == 0x0 && *(int8_t *)byte_3b18db5 == 0x0) {
                                                    sub_423d70(0x4c6, 0x0, 0x0, 0x0);
                                                    esp = (esp - 0x10) + 0x10;
                                            }
                                            *(int8_t *)ebx = 0x44;
                                            eax = sub_438410(esi + 0x4, &arg_40, arg_60 + 0x1c);
                                            bool_36 = eax != 0x8000;
                                            arg_120 = (eax > 0x8000 ? 0x1 : 0x0) + 0x6d;
                                            sub_440790(ebx);
                                            esp = (esp - 0x10) + 0x10;
                                            if (!bool_36) {
                                                    if (*(int8_t *)byte_3b18db5 == 0x0) {
                                                            sub_423d70(0xeb, 0x0, 0x0, 0x0);
                                                            sub_440930(ebx, esi, ebp, var_28);
                                                            esp = ((esp - 0x10) + 0x10 - 0xc) + 0xc;
                                                    }
                                                    else {
                                                            sub_440930(ebx, esi, ebp, var_28);
                                                            esp = (esp - 0xc) + 0xc;
                                                    }
                                            }
                                            else {
                                                    sub_440930(ebx, esi, ebp, var_28);
                                                    esp = (esp - 0xc) + 0xc;
                                            }
                                    }
                                    else {
                                            sub_43ee70(ebx, var_18, ebp);
                                            esp = (esp - 0xc) + 0xc;
                                            sub_491d70();
                                            esi = var_18;
                                            sub_440930(ebx, esi, ebp, var_28);
                                            esp = (esp - 0xc) + 0xc;
                                    }
                                    break;
                                case 49:
                                    sub_442ce0(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    sub_440930(ebx, esi, ebp, decomp_var_58);
                                    esp = (esp - 0x3c) + 0x3c;
                                    break;
                                case 50:
                                    sub_443ad0(ebx, esi, ebp);
                                    esp = esp - 0x18;
                                    sub_448af0(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x24) + 0x3c;
                                    break;
                                case 51:
                                    sub_443ad0(ebx, esi, ebp);
                                    sub_448d10(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    esp = (esp - 0x30) + 0x30;
                                    if (sub_44cdf0(ebx, esi, ebp) != 0x0) {
                                            arg_120 = 0x70;
                                    }
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0xc) + 0xc;
                                    break;
                                case 52:
                                    eax = sub_43bdc0(ebx, 0x0, &var_18);
                                    esp = (esp - 0xc) + 0xc;
                                    if (eax != 0x0) {
                                            asm { fld        dword [esp+0x28+var_18] };
                                            asm { fcomp      dword [dword_value_1142947840+4] };
                                            asm { fnstsw     ax };
                                            if (PARITY(HIBYTE(eax) & 0x44)) {
                                                    asm { fild       dword [esp+0x28+var_14] };
                                                    asm { fmul       dword [dword_value_939524096] };
                                                    asm { fmul       dword [dword_value_1199570944] };
                                                    edi = sub_644340();
                                                    sub_788290();
                                                    asm { fmul       dword [esp+0x28+var_18] };
                                                    ecx = edi;
                                                    asm { fstp       dword [esp+0x28+var_C] };
                                                    sub_7883c0();
                                                    asm { fmul       dword [esp+0x28+var_18] };
                                                    esp = esp - 0x4;
                                                    var_2C = &var_C;
                                                    var_8 = 0x0;
                                                    asm { fstp       dword [esp+0x2c+var_4] };
                                                    sub_435da0(var_2C);
                                                    esp = esp + 0x4;
                                                    eax = sub_6443bf();
                                                    asm { fild       dword [esp+0x28+var_14] };
                                                    asm { fmul       dword [dword_value_939524096] };
                                                    asm { fcomp      dword [dword_value_1114767360+4] };
                                                    asm { fnstsw     ax };
                                                    if ((HIBYTE(eax) & 0x41) == 0x0) {
                                                            asm { fld        dword [esp+0x28+var_4] };
                                                            esp = esp - 0x4;
                                                            var_2C = ecx;
                                                            asm { fmul       dword [qword_value_4604029899060858061+8] };
                                                            ecx = var_8;
                                                            edx = ebx[0x8];
                                                            asm { fstp       dword [esp+0x2c+var_2C] };
                                                            esp = esp - 0x4;
                                                            decomp_var_30 = ecx;
                                                            asm { fld        dword [esp+0x30+var_C] };
                                                            esp = esp - 0x4;
                                                            var_34 = ecx;
                                                            asm { fmul       dword [dword_value_1056964608] };
                                                            asm { fstp       dword [esp+0x34+var_34] };
                                                            sub_441750(edx, var_34, decomp_var_30, var_2C);
                                                            esp = (esp - 0x4) + 0x10;
                                                    }
                                            }
                                    }
                                    sub_4438a0(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    esp = (esp - 0x18) + 0x18;
                                    if ((ebx[0x4] & 0x1) != 0x0) {
                                            esi[0x2] = 0x0;
                                    }
                                    esp = esp - 0xc;
                                    eax = sub_43ee70(ebx, esi, ebp);
                                    esp = esp + 0xc;
                                    asm { fld        dword [ebp+0x38] };
                                    asm { fcomp      dword [ebp+0xc4] };
                                    asm { fnstsw     ax };
                                    if ((HIBYTE(eax) & 0x1) == 0x0) {
                                            sub_494050(ebx);
                                            arg_78 = arg_78 | 0x1;
                                            sub_4940b0(ebx);
                                            esp = (esp - 0x8) + 0x8;
                                    }
                                    break;
                                case 53:
                                    eax = sub_43bdc0(ebx, 0x0, &var_18);
                                    esp = (esp - 0xc) + 0xc;
                                    if (eax != 0x0) {
                                            asm { fld        dword [esp+0x28+var_18] };
                                            asm { fcomp      dword [dword_value_1142947840+4] };
                                            asm { fnstsw     ax };
                                            if (PARITY(HIBYTE(eax) & 0x44)) {
                                                    asm { fild       dword [esp+0x28+var_14] };
                                                    asm { fmul       dword [dword_value_939524096] };
                                                    asm { fmul       dword [dword_value_1199570944] };
                                                    sub_644340();
                                                    sub_788290();
                                                    asm { fmul       dword [esp+0x28+var_18] };
                                                    asm { fstp       dword [esp+0x28+var_C] };
                                                    sub_7883c0();
                                                    asm { fmul       dword [esp+0x28+var_18] };
                                                    esp = esp - 0x4;
                                                    var_2C = &var_C;
                                                    asm { fstp       dword [esp+0x2c+var_4] };
                                                    sub_435da0(var_2C);
                                                    esp = esp + 0x4;
                                            }
                                    }
                                    esp = esp - 0xc;
                                    sub_443ad0(ebx, esi, ebp);
                                    sub_443650(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    esp = esp - 0x30;
                                    eax = sub_43ee70(ebx, esi, ebp);
                                    asm { fld        dword [ebp+0x38] };
                                    asm { fcomp      dword [ebp+0xc4] };
                                    esp = esp + 0x3c;
                                    asm { fnstsw     ax };
                                    if ((HIBYTE(eax) & 0x1) == 0x0) {
                                            sub_494050(ebx);
                                            arg_78 = arg_78 | 0x1;
                                            sub_4940b0(ebx);
                                            esp = (esp - 0x8) + 0x8;
                                    }
                                    break;
                                case 54:
                                    esp = esp - 0xc;
                                    sub_44c270(ebx, esi, ebp);
                                    esp = esp - 0xc;
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x18) + 0x30;
                                    break;
                                case 55:
                                    sub_44c270(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    esp = (esp - 0x24) + 0x24;
                                    if (sub_44cdf0(ebx, esi, ebp) == 0x2) {
                                            *(int8_t *)ebx = 0x4b;
                                            arg_120 = 0x86;
                                            asm { fld        dword [ebp+0x38] };
                                            asm { fmul       dword [dword_value_3197737370] };
                                            asm { fstp       dword [ebp+0x38] };
                                            asm { fld        dword [ebp+0x40] };
                                            asm { fmul       dword [dword_value_3197737370] };
                                            asm { fstp       dword [ebp+0x40] };
                                    }
                                    else {
                                            sub_43ee70(ebx, esi, ebp);
                                            sub_491820();
                                            esp = (esp - 0x10) + 0x10;
                                    }
                                    break;
                                case 56:
                                    sub_442a50(ebx, esi, ebp);
                                    sub_4441b0(ebx, esi, ebp);
                                    sub_43e100(ebx, esi, ebp);
                                    sub_491820();
                                    sub_494080(ebx);
                                    esp = (esp - 0x2c) + 0x2c;
                                    arg_78 = arg_78 | 0x1;
                                    break;
                                case 57:
                                    sub_443ad0(ebx, esi, ebp);
                                    esp = esp - 0x18;
                                    sub_44b9c0(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x24) + 0x3c;
                                    break;
                                case 58:
                                    sub_443ad0(ebx, var_18, ebp);
                                    sub_492300(var_18);
                                    sub_44cdf0(ebx, var_18, ebp);
                                    sub_43ee70(ebx, var_18, ebp);
                                    sub_494080(ebx);
                                    arg_78 = arg_78 | 0x1;
                                    eax = sub_40b860(0x2, 0x5, sub_55fb20);
                                    esp = (esp - 0x38) + 0x38;
                                    if (eax != 0x0) {
                                            *(int8_t *)(eax[0x8] + 0x8) = ebx[0x8];
                                    }
                                    esi = var_18;
                                    break;
                                case 59:
                                    sub_4184a0(ebx);
                                    esp = (esp - 0x4) + 0x4;
                                    break;
                                case 60:
                                    if ((arg_12a & 0x1) != 0x0) {
                                            ecx = ebx[0xe];
                                            asm { fld        dword [ebp+0x1ec] };
                                            ecx = ecx[0x3];
                                            asm { fcomp      dword [ecx+0xc] };
                                            asm { fnstsw     ax };
                                            if ((HIBYTE(eax) & 0x41) == 0x0 && (arg_1e0 & 0x2) != 0x0) {
                                                    asm { fld        dword [ebp+0x1ec] };
                                                    asm { fsub       dword [ecx+0xc] };
                                                    asm { fst        dword [ebp+0x3c] };
                                                    asm { fcomp      dword [dword_value_1065336439+4] };
                                                    asm { fnstsw     ax };
                                            }
                                    }
                                    sub_443ad0(ebx, esi, ebp);
                                    esp = esp - 0x18;
                                    sub_448e50(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x24) + 0x3c;
                                    break;
                                case 61:
                                    if ((ebx[0x4] & 0x3) != 0x0) {
                                            arg_120 = 0x57;
                                    }
                                    sub_443ad0(ebx, esi, ebp);
                                    esp = esp - 0x18;
                                    sub_448e50(ebx, esi, ebp);
                                    sub_443f50(ebx, esi, ebp);
                                    sub_44cdf0(ebx, esi, ebp);
                                    sub_43ee70(ebx, esi, ebp);
                                    esp = (esp - 0x24) + 0x3c;
                                    break;
                                default:
                                    sub_492170();
                                    esp = (esp - 0x8) + 0x8;
                                    esi = var_18;
                                    break;
                            }
                    }
                    break;
            }
    }
    sub_44a800(&arg_11c);
    esp = (esp - 0x4) + 0x4;
    if ((ebx[0x2] & 0x100) == 0x0) {
            if (*(int8_t *)byte_3b18db5 == 0x0) {
                    if ((arg_0 & 0x2) != 0x0) {
                            eax = *dword_38f6e80;
                            *(eax[0x13] + 0x30) = eax[0x3a];
                            edx = *(*dword_38f6e80 + 0xf0);
                    }
                    else {
                            eax = *dword_38f6e80;
                            *(eax[0x13] + 0x30) = eax[0x14];
                            edx = *(*dword_38f6e80 + 0x3c);
                    }
                    *(*(*dword_38f6e80 + 0x38) + 0x30) = edx;
                    ecx = *(*dword_38f6e80 + 0xf8);
                    if ((arg_0 & 0x1) != 0x0) {
                            edx = *(*dword_38f6e80 + 0xfc);
                    }
                    else {
                            edx = *(*dword_38f6e80 + 0xc);
                    }
                    ecx[0xc] = edx;
            }
            sub_43fa90(ebx, esi, ebp);
            esp = (esp - 0xc) + 0xc;
    }
    if (*(int8_t *)byte_3b18db5 != 0x0) {
            if (arg_120 == 0x92) {
                    if (*(int8_t *)byte_3c55d44 == 0x0) {
                            sub_423e20(0x309, ebx, 0x1, 0x0);
                            esp = (esp - 0x10) + 0x10;
                            *(int8_t *)byte_3c55d44 = 0x1;
                    }
            }
            else {
                    if (*(int8_t *)byte_3c55d44 == 0x1) {
                            sub_424210(0x309);
                            esp = (esp - 0x4) + 0x4;
                    }
                    *(int8_t *)byte_3c55d44 = 0x0;
            }
    }
    sub_4915f0();
    sub_4948c0(arg_0);
    esp = (esp - 0x8) + 0xc;
    if (arg_64 != 0x0) {
            sub_4955e0(esi);
            esp = (esp - 0x4) + 0x4;
    }
    sub_49f1a0(ebx, &arg_1d8);
    eax = arg_122;
    esp = (esp - 0x8) + 0x8;
    switch (eax) {
        case 75:
            *(int16_t *)(ebx[0xe] + 0xa2) = 0xffff;
            *(ebx[0xe] + 0xa4) = 0x0;
            eax = ebx[0xe];
            eax[0x4] = eax[0x4] & 0xe0;
            eax = sub_446f10(ebx, esi, ebp);
            break;
        case 143:
            *(int16_t *)(ebx[0xe] + 0xa2) = 0xffff;
            *(ebx[0xe] + 0xa4) = 0x0;
            eax = ebx[0xe];
            eax[0x4] = eax[0x4] & 0xe0;
            eax = sub_446f10(ebx, esi, ebp);
            break;
        default:
            eax = *(int8_t *)ebx;
            if (eax >= 0x37 && eax <= 0x3c) {
                    *(int16_t *)(ebx[0xe] + 0xa2) = 0xffff;
                    *(ebx[0xe] + 0xa4) = 0x0;
                    eax = ebx[0xe];
                    eax[0x4] = eax[0x4] & 0xe0;
                    eax = sub_446f10(ebx, esi, ebp);
            }
            else {
                    sub_41c280(ebx);
                    esp = (esp - 0x4) + 0x4;
                    sub_787cc0();
                    asm { fadd       dword [ebp+0x110] };
                    edx = ebx[0xe];
                    var_28 = ecx;
                    asm { fstp       dword [esp+0x28+var_28] };
                    sub_41c530(edx, var_28);
                    eax = sub_446f10(ebx, esi, ebp);
            }
            break;
    }
    return eax;
