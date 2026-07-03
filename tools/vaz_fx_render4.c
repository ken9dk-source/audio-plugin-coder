// ==== @0x5218d8 -> FUN_005218d8 @0x5218d8 ====

void FUN_005218d8(int param_1,int param_2,int param_3)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  
  iVar7 = 0;
  do {
    uVar2 = *(int *)(param_1 + 0x290) + *(int *)(param_1 + 0x294);
    *(uint *)(param_1 + 0x290) = uVar2;
    iVar1 = *(int *)(param_1 + 0x310 +
                    (((int)((uVar2 ^ (int)uVar2 >> 0x1f) - ((int)uVar2 >> 0x1f)) >> 0x10) *
                     *(int *)(param_1 + 0x280) + *(int *)(param_1 + 0x264) * 0x8000 >> 0xf) * 4);
    iVar5 = (int)((ulonglong)
                  ((longlong)(*(int *)(param_2 + iVar7 * 8) << 2) *
                  (longlong)*(int *)(param_1 + 0x298)) >> 0x20);
    iVar8 = *(int *)(param_1 + 0x2a0);
    iVar4 = iVar5 + (int)((ulonglong)
                          ((longlong)(*(int *)(param_1 + 0x2a4) << 2) *
                          (longlong)*(int *)(param_1 + 0x29c)) >> 0x20);
    do {
      iVar8 = iVar8 + -1;
      iVar3 = *(int *)(param_1 + 0x2b0 + iVar8 * 4) -
              (int)((ulonglong)((longlong)(iVar4 << 2) * (longlong)iVar1) >> 0x20);
      *(int *)(param_1 + 0x2b0 + iVar8 * 4) =
           (int)((ulonglong)((longlong)(iVar3 * 4) * (longlong)iVar1) >> 0x20) + iVar4;
      iVar4 = iVar3;
    } while (iVar8 != 0);
    *(int *)(param_1 + 0x2a4) = iVar3;
    iVar5 = (int)((ulonglong)
                  ((longlong)(*(int *)(param_1 + 0x288) << 0x16) * (longlong)((iVar3 - iVar5) * 4))
                 >> 0x20) + iVar5;
    *(int *)(param_2 + iVar7 * 8) = iVar5;
    if (0xffffff < iVar5 + 0x800000U) {
      *(undefined4 *)(param_1 + 0x2ac) = 0;
    }
    uVar2 = *(int *)(param_1 + 0x290) + *(int *)(param_1 + 0x284) * -0x1000000;
    uVar6 = (int)uVar2 >> 0x1f;
    iVar1 = *(int *)(param_1 + 0x310 +
                    (((int)((uVar2 ^ uVar6) - uVar6) >> 0x10) * *(int *)(param_1 + 0x280) +
                     *(int *)(param_1 + 0x264) * 0x8000 >> 0xf) * 4);
    iVar5 = (int)((ulonglong)
                  ((longlong)(*(int *)(param_2 + 4 + iVar7 * 8) << 2) *
                  (longlong)*(int *)(param_1 + 0x298)) >> 0x20);
    iVar8 = *(int *)(param_1 + 0x2a0);
    iVar4 = iVar5 + (int)((ulonglong)
                          ((longlong)(*(int *)(param_1 + 0x2a8) << 2) *
                          (longlong)*(int *)(param_1 + 0x29c)) >> 0x20);
    do {
      iVar8 = iVar8 + -1;
      iVar3 = *(int *)(param_1 + 0x2e0 + iVar8 * 4) -
              (int)((ulonglong)((longlong)(iVar4 << 2) * (longlong)iVar1) >> 0x20);
      *(int *)(param_1 + 0x2e0 + iVar8 * 4) =
           (int)((ulonglong)((longlong)(iVar3 * 4) * (longlong)iVar1) >> 0x20) + iVar4;
      iVar4 = iVar3;
    } while (iVar8 != 0);
    *(int *)(param_1 + 0x2a8) = iVar3;
    iVar5 = (int)((ulonglong)
                  ((longlong)(*(int *)(param_1 + 0x288) << 0x16) * (longlong)((iVar3 - iVar5) * 4))
                 >> 0x20) + iVar5;
    *(int *)(param_2 + 4 + iVar7 * 8) = iVar5;
    if (0xffffff < iVar5 + 0x800000U) {
      *(undefined4 *)(param_1 + 0x2ac) = 0;
    }
    iVar7 = iVar7 + 1;
  } while (iVar7 != param_3);
  return;
}



// ==== @0x5217fc -> FUN_005217fc @0x5217fc ====

void FUN_005217fc(int param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  undefined4 uVar2;
  
  *(int *)(param_1 + 0x2ac) =
       *(int *)(param_1 + 0x2ac) + (*(uint *)(param_1 + 0x2ac) >> 0x18) * 0x800000;
  *(int *)(param_1 + 0x2ac) = *(int *)(param_1 + 0x2ac) + param_3;
  if (*(int *)(param_1 + 0x274) != 0) {
    iVar1 = FUN_004a0a68(*(undefined4 *)(param_1 + 0x28));
    if ((iVar1 != *(int *)(param_1 + 0xb10)) ||
       (*(int *)(param_1 + 0x278) != *(int *)(param_1 + 0xb14))) {
      uVar2 = FUN_004a0a68(*(undefined4 *)(param_1 + 0x28));
      *(undefined4 *)(param_1 + 0xb10) = uVar2;
      *(undefined4 *)(param_1 + 0xb14) = *(undefined4 *)(param_1 + 0x278);
      uVar2 = FUN_00402bf4();
      *(undefined4 *)(param_1 + 0x294) = uVar2;
    }
  }
  FUN_004c3ad0(param_1,param_2,param_3);
  return;
}



// ==== @0x521aa0 -> FUN_00521aa0 @0x521aa0 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00521aa0(int param_1)

{
  undefined4 uVar1;
  undefined4 extraout_ECX;
  int iVar2;
  float10 fVar3;
  float fVar4;
  
  FUN_004bed70(param_1);
  iVar2 = 0;
  do {
    fVar3 = ((float10)(iVar2 * 5) * (float10)0.6931471805599453 * _DAT_00521b40) /
            (float10)_DAT_00521b4c;
    FUN_00402ba8();
    fVar4 = (float)(((float10)_DAT_00521b54 -
                    (fVar3 * (float10)_DAT_00521b50) / (float10)**(uint **)(param_1 + 0x1c)) *
                   (float10)_DAT_00521b58);
    if (fVar4 < _DAT_00521b5c) {
      fVar4 = 0.0;
    }
    uVar1 = FUN_00402bf4();
    *(undefined4 *)(param_1 + 0x310 + iVar2 * 4) = uVar1;
    iVar2 = iVar2 + 1;
  } while (iVar2 != 0x200);
  FUN_00521c84(param_1,*(undefined4 *)(param_1 + 0x27c),extraout_ECX,fVar4);
  return;
}



// ==== @0x51b478 -> FUN_0051b478 @0x51b478 ====
// decompile failed

// ==== @0x51bf78 -> FUN_0051bf78 @0x51bf78 ====

void FUN_0051bf78(int param_1,int *param_2)

{
  uint uVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  
  FUN_004bed70(param_1,param_2);
  uVar1 = **(uint **)(param_1 + 0x1c);
  *(uint *)(param_1 + 0x2d4) = uVar1 / 1000;
  uVar2 = FUN_00402bf4(0,uVar1 % 1000,1000,**(undefined4 **)(param_1 + 0x1c),0);
  *(undefined4 *)(param_1 + 0x304) = uVar2;
  *(undefined1 *)(param_1 + 0x300) = 1;
  FUN_0051c12c(param_1,*(undefined4 *)(param_1 + 0x26c));
  FUN_0051c1cc(param_1,*(undefined4 *)(param_1 + 0x278));
  FUN_0051c5b8(param_1,*(undefined4 *)(param_1 + 0x294));
  FUN_0051c298(param_1,*(undefined4 *)(param_1 + 0x280));
  FUN_0051c684(param_1,*(undefined4 *)(param_1 + 0x29c));
  *(undefined1 *)(param_1 + 0x300) = 0;
  iVar3 = (*param_2 * 0x9f6) / 1000;
  iVar4 = 0x8000;
  if (0x8000 < iVar3) {
    do {
      iVar4 = iVar4 * 2;
    } while (iVar4 < iVar3);
  }
  if (iVar4 + -1 != *(int *)(param_1 + 0x2dc)) {
    *(int *)(param_1 + 0x2dc) = iVar4 + -1;
    if (*(int *)(param_1 + 0x2e0) != 0) {
      FUN_00402784();
    }
    uVar2 = FUN_00408588((*(int *)(param_1 + 0x2dc) + 1) * 8);
    *(undefined4 *)(param_1 + 0x2e0) = uVar2;
  }
  return;
}



// ==== @0x51c90c -> FUN_0051c90c @0x51c90c ====

void FUN_0051c90c(undefined4 param_1,undefined4 param_2)

{
  FUN_0051ce14(PTR_PTR_0051af20,1,param_1,param_2);
  return;
}



// ==== @0x51b98c -> FAIL ====

// ==== @0x517964 -> FUN_00517964 @0x517964 ====

/* WARNING: Control flow encountered bad instruction data */
/* WARNING: Instruction at (ram,0x00517af0) overlaps instruction at (ram,0x00517aef)
    */
/* WARNING: Removing unreachable block (ram,0x0051799a) */
/* WARNING: Removing unreachable block (ram,0x0051799c) */
/* WARNING: Removing unreachable block (ram,0x00517a17) */
/* WARNING: Removing unreachable block (ram,0x005179ae) */
/* WARNING: Removing unreachable block (ram,0x00517a24) */
/* WARNING: Removing unreachable block (ram,0x005179bb) */
/* WARNING: Removing unreachable block (ram,0x00517a26) */
/* WARNING: Removing unreachable block (ram,0x00517b2f) */
/* WARNING: Removing unreachable block (ram,0x00517ae4) */
/* WARNING: Removing unreachable block (ram,0x00517af1) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint * FUN_00517964(uint *param_1,byte *param_2,code *param_3)

{
  ushort *puVar1;
  byte *pbVar2;
  undefined2 *puVar3;
  short sVar4;
  byte bVar5;
  byte bVar6;
  char cVar7;
  byte *pbVar8;
  uint *puVar9;
  uint *puVar11;
  uint uVar12;
  uint extraout_ECX;
  ushort uVar13;
  byte bVar15;
  undefined2 uVar14;
  byte *extraout_EDX;
  byte *unaff_EBX;
  uint *unaff_EBP;
  undefined4 *unaff_ESI;
  uint *puVar16;
  uint *puVar17;
  uint *unaff_EDI;
  uint *puVar18;
  uint uVar19;
  undefined2 in_CS;
  undefined2 in_SS;
  undefined2 in_DS;
  undefined2 in_GS;
  undefined4 *in_FS_OFFSET;
  bool bVar20;
  bool bVar21;
  bool bVar22;
  bool bVar23;
  bool bVar24;
  bool bVar25;
  undefined1 uStack_61;
  undefined1 uStack_60;
  undefined3 uStack_5f;
  undefined1 uVar26;
  uint *in_stack_ffffffa4;
  uint *puStack_58;
  uint *in_stack_ffffffac;
  undefined4 uStack_50;
  undefined3 uStack_4f;
  byte *pbStack_4c;
  undefined4 uStack_48;
  code *pcStack_44;
  uint *puStack_40;
  undefined4 uStack_3c;
  byte *pbStack_30;
  uint *puStack_c;
  undefined1 uStack_8;
  undefined1 uStack_7;
  undefined2 uStack_6;
  uint *puStack_4;
  char *pcVar10;
  
  uVar26 = SUB41(in_stack_ffffffa4,0);
  bVar24 = false;
  bVar5 = (byte)param_1;
  *(byte *)param_1 = (byte)*param_1 | bVar5;
  *(byte *)param_1 = (byte)*param_1 - bVar5;
  pbVar8 = (byte *)((int)param_3 + 0x6d110051);
  bVar20 = *pbVar8 < bVar5;
  *pbVar8 = *pbVar8 - bVar5;
  uVar14 = SUB42(param_2,0);
  bVar6 = (byte)param_2;
  puVar11 = unaff_EBP;
  if (bVar20) {
    puVar16 = unaff_ESI + 1;
    out(*unaff_ESI,uVar14);
    puStack_4 = param_1;
    if (bVar20) {
      pbVar8 = (byte *)((int)param_1 + (int)param_2 * 2);
      bVar20 = CARRY1(*pbVar8,bVar6);
      bVar22 = SCARRY1(*pbVar8,bVar6);
      *pbVar8 = *pbVar8 + bVar6;
      bVar23 = *pbVar8 == 0;
      puVar18 = unaff_EDI;
code_r0x005179e2:
      bVar21 = true;
      if (bVar20) goto code_r0x00517a4e;
      puVar11 = puVar16 + 1;
      out(*puVar16,uVar14);
      puVar16 = puVar16 + 2;
      out(*puVar11,uVar14);
      if (bVar22) {
        puVar9 = (uint *)((int)param_3 + -1);
        puVar11 = (uint *)param_3;
      }
      else {
        puVar9 = (uint *)param_3;
        puVar11 = param_1;
        if (!bVar23) {
          uVar19 = in(uVar14);
          *puVar18 = uVar19;
          unaff_EDI = (uint *)((int)puVar18 + 5);
          bVar5 = in(uVar14);
          *(byte *)(puVar18 + 1) = bVar5;
          puVar11 = unaff_EBP;
          in_stack_ffffffa4 = param_1;
          goto code_r0x005179f9;
        }
      }
code_r0x00517a54:
      pbVar8 = (byte *)((int)param_1 + -1);
      param_1 = (uint *)CONCAT22((short)((uint)pbVar8 >> 0x10),
                                 CONCAT11((char)((uint)pbVar8 >> 8) + (char)((uint)puVar9 >> 8),
                                          (char)pbVar8));
      param_2 = (byte *)0x52a40048;
      param_3 = (code *)((int)puVar9 + -1);
      *(byte *)(puVar9 + -0x16adffca) = (byte)puVar9[-0x16adffca] + (char)unaff_EBX;
      *(byte *)param_1 = (byte)*param_1 + (char)param_3;
      bVar24 = ((uint)puVar11 & 0x400) != 0;
      puVar11 = unaff_EBP;
      goto code_r0x00517a65;
    }
    pbVar8 = (byte *)(unaff_ESI + 2);
    out(*puVar16,uVar14);
    uStack_8 = 0x61;
    uStack_7 = 0x6e;
    uStack_6 = 0x6567;
    *(byte *)param_1 = (byte)*param_1 & bVar5;
    while( true ) {
      bVar21 = 0xb28c92e6 < *(uint *)param_3;
      uVar19 = *(uint *)param_3;
      *(uint *)param_3 = *(uint *)param_3 + 0x4d736d19;
      out(*(undefined4 *)pbVar8,uVar14);
      puVar16 = (uint *)(pbVar8 + 4);
      puStack_58 = param_1;
      if (!bVar21) {
        puVar16 = (uint *)(pbVar8 + 8);
        out(*(uint *)(pbVar8 + 4),uVar14);
        in_stack_ffffffa4 = unaff_EBP;
        if (!SCARRY4(uVar19,0x4d736d19)) {
          param_2 = (byte *)uStack_48;
          puVar18 = unaff_EBP;
          if (*(uint *)param_3 != 0) {
            puVar17 = param_1 + 1;
            out(*param_1,(short)uStack_48);
            puVar9 = puStack_40;
            param_3 = pcStack_44;
            unaff_EBX = pbStack_4c;
            puVar11 = in_stack_ffffffac;
            goto code_r0x00517a05;
          }
          bVar5 = (byte)((uint)pcStack_44 >> 8);
          bVar20 = CARRY1(bVar5,*(byte *)((int)in_stack_ffffffac + 0x73));
          param_3 = (code *)CONCAT22((short)((uint)pcStack_44 >> 0x10),
                                     CONCAT11(bVar5 + *(byte *)((int)in_stack_ffffffac + 0x73),
                                              (char)pcStack_44));
          puVar11 = (uint *)((int)in_stack_ffffffac + -1);
          puVar9 = puStack_40;
          unaff_EBX = pbStack_4c;
          puVar16 = param_1;
          goto code_r0x005179ff;
        }
      }
      if (SCARRY4(uVar19,0x4d736d19) == (int)*(uint *)param_3 < 0) break;
      pbVar8 = (byte *)((int)puVar16 + 1);
      out((byte)*puVar16,uVar14);
      pbVar2 = (byte *)segment(in_GS,(short)unaff_EBX + 1 + (short)pbVar8);
      *pbVar2 = *pbVar2 & bVar5;
    }
code_r0x005179f9:
    unaff_EBX = unaff_EBX + 1;
    uStack_3c = (uint *)param_3;
    bVar20 = CARRY1(*param_2,bVar6);
    *param_2 = *param_2 + bVar6;
    puVar18 = unaff_EDI + 1;
    uVar19 = in(uVar14);
    *unaff_EDI = uVar19;
    puVar9 = param_1;
    unaff_EBP = puVar11;
    bVar21 = bVar20;
    if (!bVar20) goto code_r0x00517a4c;
code_r0x005179ff:
    puVar17 = puVar16 + 1;
    out(*puVar16,(short)param_2);
    bVar21 = false;
    unaff_EBP = in_stack_ffffffa4;
    if (bVar20) goto code_r0x00517a6e;
code_r0x00517a05:
    uVar26 = SUB41(unaff_EBP,0);
    out(*puVar17,(short)param_2);
    unaff_EBX = unaff_EBX + -1;
    param_1 = puVar9;
    puVar9 = (uint *)param_3;
    unaff_EBP = puVar11;
    puVar16 = puVar17 + 1;
    if (-1 < (int)unaff_EBX) goto code_r0x00517a4f;
    puVar9 = puVar17 + 2;
    out(puVar17[1],(short)param_2);
    if (bVar21 || unaff_EBX == (byte *)0x0) goto code_r0x00517a0e;
code_r0x00517a7c:
    if (param_3 == (code *)0xffffffff) {
      uVar14 = SUB42(param_2,0);
      out(*puVar9,uVar14);
      out((byte)puVar9[(uint)bVar24 * -2 + 1],uVar14);
      out(*(undefined4 *)((int)(puVar9 + (uint)bVar24 * -2 + 1) + (uint)bVar24 * -2 + 2),uVar14);
      if (bVar21) {
        return puStack_c;
      }
                    /* WARNING: Bad instruction - Truncating control flow here */
      halt_baddata();
    }
    puStack_40 = (uint *)0x517af9;
    puVar9 = (uint *)FUN_004037c8();
    puVar11[-1] = extraout_ECX;
    param_2 = extraout_EDX;
  }
  else {
    out(*unaff_ESI,uVar14);
    puVar16 = (uint *)((int)unaff_ESI + 5);
    out(*(undefined1 *)(unaff_ESI + 1),uVar14);
    bVar22 = param_1 < (uint *)*param_1;
    bVar23 = (int)param_1 - *param_1 < (uint)bVar20;
    bVar21 = bVar22 || bVar23;
    bVar20 = (int)param_1 - *param_1 == (uint)bVar20;
    param_1 = (uint *)0x14005181;
    puVar18 = unaff_EDI + 1;
    uVar19 = in(uVar14);
    *unaff_EDI = uVar19;
    if (bVar22 || bVar23) {
      out(*puVar16,uVar14);
      puStack_4 = (uint *)0x14005181;
      puVar11 = param_1;
      pbVar8 = param_2;
      puVar9 = (uint *)((int)unaff_ESI + 9);
      if (bVar22 || bVar23) goto code_r0x00517a38;
      out(*(uint *)((int)unaff_ESI + 9),uVar14);
      bVar25 = SBORROW4((int)unaff_EBP,1);
      unaff_EBP = (uint *)((int)unaff_EBP + -1);
      out(*(undefined4 *)((int)unaff_ESI + 0xd),uVar14);
      bVar20 = false;
      puVar16 = (uint *)((int)unaff_ESI + 0x11);
      if (unaff_EBP == (uint *)0x0) {
        bVar25 = SCARRY4((int)&puStack_4,1);
        bVar20 = &stack0x00000000 == (undefined1 *)0x3;
        puVar16 = (uint *)((int)unaff_ESI + 0x15);
        out(*(uint *)((int)unaff_ESI + 0x11),uVar14);
        if ((bVar22 || bVar23) || bVar20) {
          bVar20 = 0x14005181 < uRam14005181;
          bVar22 = SBORROW4(0x14005181,uRam14005181);
          param_1 = (uint *)(0x14005181 - uRam14005181);
          bVar23 = param_1 == (uint *)0x0;
          uStack_7 = 0x82;
          uStack_6 = 0x51;
          puStack_4 = (uint *)0x14005114;
          puStack_c._1_3_ = SUB43(param_1,0);
          uStack_8 = (undefined1)((uint)param_1 >> 0x18);
          goto code_r0x005179e2;
        }
      }
      if (bVar20) {
        puStack_40 = puVar16 + 1;
        out(*puVar16,uVar14);
        puVar9 = puStack_40;
        if (bVar25) goto code_r0x00517b1f;
        goto code_r0x00517abe;
      }
code_r0x00517a4a:
      if (bVar25) {
        halt_baddata();
      }
code_r0x00517a4c:
      out((byte)*puVar16,(short)param_2);
      puVar16 = (uint *)((int)puVar16 + 2);
code_r0x00517a4e:
      out(*puVar16,(short)param_2);
      puVar9 = (uint *)param_3;
      puVar16 = puVar16 + 1;
code_r0x00517a4f:
      puStack_40 = puVar16;
      if (bVar21) {
code_r0x00517abe:
        out((byte)*puStack_40,(short)param_2);
                    /* WARNING: Bad instruction - Truncating control flow here */
        halt_baddata();
      }
      *(byte *)param_1 = (byte)*param_1 | (byte)param_1;
      puVar16 = puStack_40;
      puVar11 = puVar18;
      uStack_48 = (uint *)param_2;
      pcStack_44 = (code *)puVar9;
      goto code_r0x00517a54;
    }
    if (!bVar20) {
      puVar9 = (uint *)((int)unaff_ESI + 9);
      out(*puVar16,uVar14);
code_r0x00517a0e:
      bVar6 = (byte)param_1;
      bVar15 = (byte)unaff_EBX;
      param_1 = (uint *)CONCAT31((int3)((uint)param_1 >> 8),bVar6 + bVar15);
      uVar19 = *(uint *)param_3;
      bVar5 = (byte)*(uint *)param_3 + 0xf;
      *param_3 = (code)(bVar5 + CARRY1(bVar6,bVar15));
      if (-1 < (char)(byte)*(uint *)param_3) {
        *(byte *)((int)param_1 + 0x39) = *(byte *)((int)param_1 + 0x39) + (char)((uint)param_3 >> 8)
        ;
        *(byte *)puVar9 = (byte)*puVar9 + bVar15;
        puVar9[0x16] = puVar9[0x16] | (uint)param_1;
        if (param_3 == (code *)0xffffffff) {
          out(*puVar9,(short)param_2);
          out((byte)puVar9[1],(short)param_2);
          *(byte *)param_1 = (byte)*param_1 + bVar6 + bVar15;
          puStack_40 = (uint *)&UNK_00517aae;
          puVar11 = (uint *)FUN_00517ae4(_UNK_00517508,1,param_1);
          return puVar11;
        }
        goto code_r0x00517b0c;
      }
      puVar1 = (ushort *)(unaff_EBX + 0x68);
      *puVar1 = *puVar1 + (ushort)(0xf0 < (byte)uVar19 || CARRY1(bVar5,CARRY1(bVar6,bVar15))) *
                          (((ushort)param_1 & 3) - (*puVar1 & 3));
      puVar18 = (uint *)CONCAT13((char)in_stack_ffffffac,puStack_58._1_3_);
      param_3 = (code *)CONCAT13(pbStack_4c._0_1_,uStack_4f);
      uVar13 = (ushort)((uint)in_stack_ffffffac >> 8);
      out(*(undefined1 *)CONCAT13(uStack_60,CONCAT12(uStack_61,in_SS)),uVar13);
      puVar3 = (undefined2 *)segment(in_SS,(short)((int)&uStack_48 + 1) + -2);
      *puVar3 = in_DS;
      bVar6 = (byte)((uint)pbStack_4c >> 8);
      bVar15 = (byte)((uint)in_stack_ffffffac >> 0x10);
      puVar11 = (uint *)CONCAT31((int3)(CONCAT13((undefined1)uStack_48,pbStack_4c._1_3_) >> 8),
                                 bVar6 + bVar15);
      bVar5 = (byte)*(uint *)param_3 + 0x17;
      bVar21 = 0xe8 < (byte)*(uint *)param_3 || CARRY1(bVar5,CARRY1(bVar6,bVar15));
      *param_3 = (code)(bVar5 + CARRY1(bVar6,bVar15));
      if (-1 < (char)(byte)*(uint *)param_3) goto code_r0x00517aa8;
      puVar1 = (ushort *)(CONCAT13(uVar26,uStack_5f) + 0x70);
      sVar4 = (uVar13 & 3) - (*puVar1 & 3);
      bVar20 = 0 < sVar4;
      *puVar1 = *puVar1 + (ushort)bVar21 * sVar4;
      puVar11 = puStack_40;
      param_3 = pcStack_44;
      pbVar8 = (byte *)uStack_48;
      unaff_EBX = pbStack_4c;
      unaff_EBP = in_stack_ffffffac;
      puVar9 = puStack_58;
code_r0x00517a38:
      if (bVar20) {
        unaff_EBX[-0x5efe4d38] = unaff_EBX[-0x5efe4d38];
code_r0x00517aa8:
        cVar7 = (char)puVar11 + (char)((uint)param_3 >> 8);
        pcVar10 = (char *)CONCAT31((int3)((uint)puVar11 >> 8),cVar7);
        *pcVar10 = *pcVar10 + cVar7;
                    /* WARNING: Bad instruction - Truncating control flow here */
        halt_baddata();
      }
      uVar14 = SUB42(pbVar8,0);
      out(*puVar9,uVar14);
      if (!bVar21) goto code_r0x00517aa8;
      out(puVar9[1],uVar14);
      out((byte)puVar9[2],uVar14);
      param_1 = &uStack_3c;
      uStack_3c = (uint *)CONCAT22((short)((uint)puVar11 >> 0x10),in_CS);
      param_2 = pbStack_30;
      if (param_3 != (code *)0xffffffff) goto code_r0x00517abe;
      puVar16 = (uint *)((int)puVar9 + 0xe);
      out(*(undefined4 *)((int)puVar9 + 10),uVar14);
      param_3 = (code *)0x0;
      bVar25 = false;
      bVar21 = true;
      param_2 = pbVar8;
      goto code_r0x00517a4a;
    }
code_r0x00517a65:
    puVar9 = (uint *)((int)param_1 + -1);
    (&stack0x409c0049)[(int)param_2 * 4] =
         (&stack0x409c0049)[(int)param_2 * 4] + (char)((uint)puVar9 >> 8);
    puVar17 = (uint *)((int)puVar16 + 1);
code_r0x00517a6e:
    puVar16 = puVar9 + -0x1bffee72;
    *(byte *)puVar16 = (byte)*puVar16 + (char)((uint)puVar9 >> 8);
    bVar5 = (byte)puVar9;
    if (-1 < (char)(byte)*puVar16) {
      puStack_4 = (uint *)param_3;
      bVar21 = CARRY1((byte)*puVar18,bVar5);
      *(byte *)puVar18 = (byte)*puVar18 + bVar5;
      puStack_c = (uint *)&uStack_8;
      uStack_8 = (undefined1)in_CS;
      uStack_7 = (undefined1)((ushort)in_CS >> 8);
      puVar9 = (uint *)((int)puVar17 + 1);
      goto code_r0x00517a7c;
    }
    unaff_EBX[-0x30170f3c] = unaff_EBX[-0x30170f3c] | bVar5;
  }
  puStack_40 = (uint *)0x517b0c;
  param_1 = (uint *)FUN_004c398c(puVar9,0,puVar11[-1]);
  unaff_EBX = param_2;
  unaff_EBP = puVar11;
code_r0x00517b0c:
  puVar9[0x16] = 8;
  puVar9[0x17] = 0x12d;
  puVar18 = (uint *)0x0;
  do {
    unaff_EBP[-2] = (uint)puVar18;
code_r0x00517b1f:
    *(float10 *)(unaff_EBP + -5) = SQRT((float10)(int)unaff_EBP[-2] / (float10)_DAT_00517c38);
    puStack_40 = (uint *)0x517b48;
    param_1 = (uint *)FUN_00402bf4(param_1);
    puVar9[(int)(puVar18 + 0x28)] = (uint)param_1;
    puVar18 = (uint *)((int)puVar18 + 1);
  } while (puVar18 != (uint *)0x100);
  puVar9[0x1a0] = puVar9[0x19f];
  uVar19 = 0;
  do {
    unaff_EBP[-2] = uVar19;
    puStack_40 = (uint *)0x517b7f;
    FUN_00402bc0();
    puStack_40 = (uint *)0x517b96;
    uVar12 = FUN_00402bf4();
    puVar9[uVar19 + 0x1a1] = uVar12;
    uVar19 = uVar19 + 1;
  } while (uVar19 != 0x101);
  puStack_40 = (uint *)0x517bb2;
  FUN_00517e6c(puVar9,0xff);
  puVar9[0x9b] = 0x60;
  puStack_40 = (uint *)0x517bc9;
  FUN_004a2bac(puVar9[5],&DAT_0052b454);
  pcStack_44 = FUN_00517e34;
  uStack_48 = puVar9;
  puStack_40 = puVar9;
  (**(code **)(*(int *)puVar9[5] + 8))();
  (**(code **)(*(int *)puVar9[5] + 8))();
  (**(code **)(*(int *)puVar9[5] + 8))();
  (**(code **)(*(int *)puVar9[5] + 8))();
  if ((char)unaff_EBX != '\0') {
    FUN_00403820(puVar9);
    *in_FS_OFFSET = FUN_00517f70;
  }
  return puVar9;
}



// ==== @0x517c70 -> FUN_00517c70 @0x517c70 ====

void FUN_00517c70(int param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  undefined4 uVar2;
  
  if (*(int *)(param_1 + 0x268) != 0) {
    iVar1 = FUN_004a0a68(*(undefined4 *)(param_1 + 0x28));
    if ((iVar1 != *(int *)(param_1 + 0xa88)) ||
       (*(int *)(param_1 + 0x26c) != *(int *)(param_1 + 0xa8c))) {
      uVar2 = FUN_004a0a68(*(undefined4 *)(param_1 + 0x28));
      *(undefined4 *)(param_1 + 0xa88) = uVar2;
      *(undefined4 *)(param_1 + 0xa8c) = *(undefined4 *)(param_1 + 0x26c);
      uVar2 = FUN_00402bf4();
      *(undefined4 *)(param_1 + 0x27c) = uVar2;
    }
  }
  FUN_004c3ad0(param_1,param_2,param_3);
  return;
}



// ==== @0x517d34 -> FUN_00517d34 @0x517d34 ====

undefined4 FUN_00517d34(int param_1,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  longlong lVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  
  iVar7 = 0;
  do {
    uVar4 = *(int *)(param_1 + 0x278) + *(int *)(param_1 + 0x27c);
    *(uint *)(param_1 + 0x278) = uVar4;
    if (*(int *)(param_1 + 0x274) == 0) {
      iVar5 = ((int)uVar4 >> 9 ^ (int)uVar4 >> 0x1f) - ((int)uVar4 >> 0x1f);
    }
    else {
      iVar5 = *(int *)(param_1 + 0x684 + (uVar4 >> 0x18) * 4);
      iVar5 = (int)((ulonglong)
                    ((longlong)(int)((uVar4 & 0xffffff) << 6) *
                    (longlong)((*(int *)(param_1 + 0x688 + (uVar4 >> 0x18) * 4) - iVar5) * 4)) >>
                   0x20) + iVar5;
    }
    uVar4 = iVar5 * (*(int *)(param_1 + 0x264) - *(int *)(param_1 + 0x260)) +
            *(int *)(param_1 + 0x260) * 0x400000;
    iVar6 = (uVar4 & 0x3fffff) << 8;
    uVar4 = uVar4 >> 0x16;
    iVar5 = *(int *)(param_1 + 0x280 + uVar4 * 4);
    iVar1 = *(int *)(param_1 + 0x284 + uVar4 * 4);
    iVar2 = *(int *)(param_1 + 0x284 + (0xff - uVar4) * 4);
    *(int *)(param_2 + iVar7 * 8) =
         (int)((ulonglong)
               ((longlong)(*(int *)(param_2 + iVar7 * 8) << 2) *
               (longlong)
               (iVar2 + (int)((ulonglong)
                              ((longlong)(*(int *)(param_1 + 0x280 + (0xff - uVar4) * 4) - iVar2) *
                              (longlong)iVar6) >> 0x20) * 4)) >> 0x20);
    lVar3 = (longlong)(*(int *)(param_2 + 4 + iVar7 * 8) << 2) *
            (longlong)
            (iVar5 + (int)((ulonglong)((longlong)(iVar1 - iVar5) * (longlong)iVar6) >> 0x20) * 4);
    *(int *)(param_2 + 4 + iVar7 * 8) = (int)((ulonglong)lVar3 >> 0x20);
    iVar7 = iVar7 + 1;
  } while (iVar7 != param_3);
  return (int)lVar3;
}



// ==== @0x51dbcc -> FUN_0051dbcc @0x51dbcc ====

undefined4 FUN_0051dbcc(int param_1,int param_2,int param_3)

{
  longlong lVar1;
  int iVar2;
  int iVar3;
  
  iVar3 = 0;
  do {
    iVar2 = (*(uint *)(param_1 + 0x268) & 0x7ff) + *(int *)(param_1 + 0x26c);
    *(int *)(param_1 + 0x268) = iVar2;
    if (0x7ff < iVar2) {
      *(uint *)(param_1 + 0x278) =
           (*(uint *)(param_2 + iVar3 * 4) & *(uint *)(param_1 + 0x270)) + *(int *)(param_1 + 0x274)
      ;
      *(uint *)(param_1 + 0x27c) =
           (*(uint *)(param_2 + 4 + iVar3 * 4) & *(uint *)(param_1 + 0x270)) +
           *(int *)(param_1 + 0x274);
    }
    iVar2 = (int)((ulonglong)
                  ((longlong)*(int *)(param_1 + 0x280) *
                  (longlong)((*(int *)(param_1 + 0x284) + *(int *)(param_1 + 0x278)) * 0x10)) >>
                 0x20);
    *(int *)(param_2 + iVar3 * 4) = iVar2;
    *(int *)(param_1 + 0x284) = iVar2 - *(int *)(param_1 + 0x278);
    lVar1 = (longlong)*(int *)(param_1 + 0x280) *
            (longlong)((*(int *)(param_1 + 0x288) + *(int *)(param_1 + 0x27c)) * 0x10);
    iVar2 = (int)((ulonglong)lVar1 >> 0x20);
    *(int *)(param_2 + 4 + iVar3 * 4) = iVar2;
    *(int *)(param_1 + 0x288) = iVar2 - *(int *)(param_1 + 0x27c);
    iVar3 = iVar3 + 2;
    param_3 = param_3 + -1;
  } while (param_3 != 0);
  return (int)lVar1;
}



// ==== @0x51dc7c -> FUN_0051dc7c @0x51dc7c ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0051dc7c(int param_1)

{
  undefined4 uVar1;
  
  FUN_004bed70(param_1);
  FUN_0051dd14(param_1,*(undefined4 *)(param_1 + 0x260));
  FUN_00402b98();
  uVar1 = FUN_00402bf4();
  *(undefined4 *)(param_1 + 0x280) = uVar1;
  return;
}



// ==== @0x51da50 -> FUN_0051da50 @0x51da50 ====
// decompile failed

// ==== @0x51dabc -> FAIL ====

