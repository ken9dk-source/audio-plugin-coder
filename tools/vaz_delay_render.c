// ==== @0x51b97c -> FUN_0051b97c @0x51b97c ====

void FUN_0051b97c(void)

{
  int iVar1;
  uint extraout_EDX;
  
  iVar1 = FUN_00403830();
  if (*(int *)(iVar1 + 0x2e0) != 0) {
    FUN_00402784();
  }
  FUN_004c39e8(iVar1,extraout_EDX & 0xfffffffc);
  if ('\0' < (char)extraout_EDX) {
    FUN_00403818(iVar1);
  }
  return;
}



// ==== @0x51b478 -> FUN_0051b478 @0x51b478 ====
// decompile failed

// ==== @0x51bba8 -> FUN_0051bba8 @0x51bba8 ====

undefined4 FUN_0051bba8(int param_1,int *param_2,int param_3)

{
  int *piVar1;
  int iVar2;
  longlong lVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  
  piVar1 = param_2 + param_3 * 2;
  iVar2 = *(int *)(param_1 + 0x2e0);
  uVar9 = *(uint *)(param_1 + 0x2c8);
  if (*(int *)(param_1 + 0x260) == 1) {
    do {
      uVar9 = uVar9 - 1 & *(uint *)(param_1 + 0x2dc);
      iVar6 = *param_2;
      iVar8 = *(int *)(iVar2 + (*(int *)(param_1 + 0x2cc) + uVar9 & *(uint *)(param_1 + 0x2dc)) * 8)
      ;
      iVar5 = *(int *)(iVar2 + 4 +
                      (*(int *)(param_1 + 0x2d0) + uVar9 & *(uint *)(param_1 + 0x2dc)) * 8);
      iVar7 = iVar6 + (int)((ulonglong)
                            ((longlong)(iVar5 << 2) * (longlong)(*(int *)(param_1 + 0x27c) << 0x16))
                           >> 0x20);
      iVar7 = (int)((ulonglong)
                    ((longlong)((*(int *)(param_1 + 0x2b0) - iVar7) * 0x10) *
                    (longlong)*(int *)(param_1 + 0x2a8)) >> 0x20) + iVar7;
      *(int *)(param_1 + 0x2b0) = iVar7;
      *(int *)(iVar2 + uVar9 * 8) = iVar7;
      iVar6 = (int)((ulonglong)((longlong)*(int *)(param_1 + 0x2c0) * (longlong)(iVar6 << 2)) >>
                   0x20) +
              (int)((ulonglong)((longlong)(iVar8 << 2) * (longlong)*(int *)(param_1 + 0x2b8)) >>
                   0x20);
      *param_2 = iVar6;
      if (0xffffff < iVar6 + 0x800000U) {
        *(undefined4 *)(param_1 + 0x2d8) = 0;
      }
      iVar6 = param_2[1];
      iVar8 = iVar6 + (int)((ulonglong)
                            ((longlong)(iVar8 << 2) * (longlong)(*(int *)(param_1 + 0x298) << 0x16))
                           >> 0x20);
      iVar8 = (int)((ulonglong)
                    ((longlong)((*(int *)(param_1 + 0x2b4) - iVar8) * 0x10) *
                    (longlong)*(int *)(param_1 + 0x2ac)) >> 0x20) + iVar8;
      *(int *)(param_1 + 0x2b4) = iVar8;
      *(int *)(iVar2 + 4 + uVar9 * 8) = iVar8;
      lVar3 = (longlong)(iVar5 << 2) * (longlong)*(int *)(param_1 + 700);
      uVar4 = (undefined4)lVar3;
      iVar6 = (int)((ulonglong)((longlong)*(int *)(param_1 + 0x2c4) * (longlong)(iVar6 << 2)) >>
                   0x20) + (int)((ulonglong)lVar3 >> 0x20);
      param_2[1] = iVar6;
      if (0xffffff < iVar6 + 0x800000U) {
        *(undefined4 *)(param_1 + 0x2d8) = 0;
      }
      param_2 = param_2 + 2;
    } while ((int)param_2 < (int)piVar1);
  }
  else if (*(int *)(param_1 + 0x260) < 2) {
    do {
      uVar9 = uVar9 - 1 & *(uint *)(param_1 + 0x2dc);
      iVar6 = *param_2;
      iVar8 = *(int *)(iVar2 + (*(int *)(param_1 + 0x2cc) + uVar9 & *(uint *)(param_1 + 0x2dc)) * 8)
      ;
      iVar5 = iVar6 + (int)((ulonglong)
                            ((longlong)(iVar8 << 2) * (longlong)(*(int *)(param_1 + 0x27c) << 0x16))
                           >> 0x20);
      iVar5 = (int)((ulonglong)
                    ((longlong)((*(int *)(param_1 + 0x2b0) - iVar5) * 0x10) *
                    (longlong)*(int *)(param_1 + 0x2a8)) >> 0x20) + iVar5;
      *(int *)(param_1 + 0x2b0) = iVar5;
      *(int *)(iVar2 + uVar9 * 8) = iVar5;
      iVar6 = (int)((ulonglong)((longlong)*(int *)(param_1 + 0x2c0) * (longlong)(iVar6 << 2)) >>
                   0x20) +
              (int)((ulonglong)((longlong)(iVar8 << 2) * (longlong)*(int *)(param_1 + 0x2b8)) >>
                   0x20);
      *param_2 = iVar6;
      if (0xffffff < iVar6 + 0x800000U) {
        *(undefined4 *)(param_1 + 0x2d8) = 0;
      }
      iVar6 = param_2[1];
      iVar8 = *(int *)(iVar2 + 4 +
                      (*(int *)(param_1 + 0x2d0) + uVar9 & *(uint *)(param_1 + 0x2dc)) * 8);
      iVar5 = iVar6 + (int)((ulonglong)
                            ((longlong)(iVar8 << 2) * (longlong)(*(int *)(param_1 + 0x298) << 0x16))
                           >> 0x20);
      iVar5 = (int)((ulonglong)
                    ((longlong)((*(int *)(param_1 + 0x2b4) - iVar5) * 0x10) *
                    (longlong)*(int *)(param_1 + 0x2ac)) >> 0x20) + iVar5;
      *(int *)(param_1 + 0x2b4) = iVar5;
      *(int *)(iVar2 + 4 + uVar9 * 8) = iVar5;
      lVar3 = (longlong)(iVar8 << 2) * (longlong)*(int *)(param_1 + 700);
      uVar4 = (undefined4)lVar3;
      iVar6 = (int)((ulonglong)((longlong)*(int *)(param_1 + 0x2c4) * (longlong)(iVar6 << 2)) >>
                   0x20) + (int)((ulonglong)lVar3 >> 0x20);
      param_2[1] = iVar6;
      if (0xffffff < iVar6 + 0x800000U) {
        *(undefined4 *)(param_1 + 0x2d8) = 0;
      }
      param_2 = param_2 + 2;
    } while ((int)param_2 < (int)piVar1);
  }
  else {
    do {
      uVar9 = uVar9 - 1 & *(uint *)(param_1 + 0x2dc);
      iVar6 = *param_2;
      iVar8 = param_2[1];
      iVar5 = *(int *)(iVar2 + (*(int *)(param_1 + 0x2cc) + uVar9 & *(uint *)(param_1 + 0x2dc)) * 8)
      ;
      iVar7 = iVar6 + iVar8 +
              (int)((ulonglong)
                    ((longlong)(iVar5 << 2) * (longlong)(*(int *)(param_1 + 0x27c) << 0x16)) >> 0x20
                   );
      iVar7 = (int)((ulonglong)
                    ((longlong)((*(int *)(param_1 + 0x2b0) - iVar7) * 0x10) *
                    (longlong)*(int *)(param_1 + 0x2a8)) >> 0x20) + iVar7;
      *(int *)(param_1 + 0x2b0) = iVar7;
      *(int *)(iVar2 + uVar9 * 8) = iVar7;
      iVar8 = (int)((ulonglong)
                    ((longlong)*(int *)(param_1 + 0x2c0) * (longlong)((iVar6 + iVar8) * 4)) >> 0x20)
              + (int)((ulonglong)((longlong)(iVar5 << 2) * (longlong)*(int *)(param_1 + 0x2b8)) >>
                     0x20);
      iVar6 = *(int *)(iVar2 + 4 +
                      (*(int *)(param_1 + 0x2d0) + uVar9 & *(uint *)(param_1 + 0x2dc)) * 8);
      iVar5 = iVar8 + (int)((ulonglong)
                            ((longlong)(iVar6 << 2) * (longlong)(*(int *)(param_1 + 0x298) << 0x16))
                           >> 0x20);
      iVar5 = (int)((ulonglong)
                    ((longlong)((*(int *)(param_1 + 0x2b4) - iVar5) * 0x10) *
                    (longlong)*(int *)(param_1 + 0x2ac)) >> 0x20) + iVar5;
      *(int *)(param_1 + 0x2b4) = iVar5;
      *(int *)(iVar2 + 4 + uVar9 * 8) = iVar5;
      lVar3 = (longlong)(iVar6 << 2) * (longlong)*(int *)(param_1 + 700);
      uVar4 = (undefined4)lVar3;
      iVar6 = (int)((ulonglong)((longlong)*(int *)(param_1 + 0x2c4) * (longlong)(iVar8 * 4)) >> 0x20
                   ) + (int)((ulonglong)lVar3 >> 0x20);
      *param_2 = iVar6;
      param_2[1] = iVar6;
      if (0xffffff < iVar6 + 0x800000U) {
        *(undefined4 *)(param_1 + 0x2d8) = 0;
      }
      param_2 = param_2 + 2;
    } while ((int)param_2 < (int)piVar1);
  }
  *(uint *)(param_1 + 0x2c8) = uVar9;
  return uVar4;
}



// ==== @0x51b9b0 -> FUN_0051b9b0 @0x51b9b0 ====

void FUN_0051b9b0(int param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  undefined4 uVar2;
  
  *(int *)(param_1 + 0x2d8) =
       *(int *)(param_1 + 0x2d8) + (*(uint *)(param_1 + 0x2d8) >> 0x18) * 0x800000;
  *(int *)(param_1 + 0x2d8) = *(int *)(param_1 + 0x2d8) + param_3;
  if (*(int *)(param_1 + 0x268) != 0) {
    iVar1 = FUN_004a0a68(*(undefined4 *)(param_1 + 0x28));
    if (((iVar1 != *(int *)(param_1 + 0x2e8)) ||
        (*(int *)(param_1 + 0x270) != *(int *)(param_1 + 0x2ec))) ||
       (*(int *)(param_1 + 0x274) != *(int *)(param_1 + 0x2f0))) {
      uVar2 = FUN_004a0a68(*(undefined4 *)(param_1 + 0x28));
      *(undefined4 *)(param_1 + 0x2e8) = uVar2;
      *(undefined4 *)(param_1 + 0x2ec) = *(undefined4 *)(param_1 + 0x270);
      *(undefined4 *)(param_1 + 0x2f0) = *(undefined4 *)(param_1 + 0x274);
      *(undefined4 *)(param_1 + 0x308) = 0;
      *(undefined4 *)(param_1 + 0x30c) = *(undefined4 *)(param_1 + 0x2cc);
      iVar1 = FUN_00402bf4();
      *(int *)(param_1 + 0x2cc) = iVar1;
      if (*(int *)(param_1 + 0x2dc) < iVar1) {
        *(int *)(param_1 + 0x2cc) = *(int *)(param_1 + 0x2dc);
      }
      *(undefined4 *)(param_1 + 0x2e4) = 1;
    }
    iVar1 = FUN_004a0a68(*(undefined4 *)(param_1 + 0x28));
    if (((iVar1 != *(int *)(param_1 + 0x2f4)) ||
        (*(int *)(param_1 + 0x28c) != *(int *)(param_1 + 0x2f8))) ||
       (*(int *)(param_1 + 0x290) != *(int *)(param_1 + 0x2fc))) {
      uVar2 = FUN_004a0a68(*(undefined4 *)(param_1 + 0x28));
      *(undefined4 *)(param_1 + 0x2f4) = uVar2;
      *(undefined4 *)(param_1 + 0x2f8) = *(undefined4 *)(param_1 + 0x28c);
      *(undefined4 *)(param_1 + 0x2fc) = *(undefined4 *)(param_1 + 0x290);
      *(undefined4 *)(param_1 + 0x310) = 0;
      *(undefined4 *)(param_1 + 0x314) = *(undefined4 *)(param_1 + 0x2d0);
      iVar1 = FUN_00402bf4();
      *(int *)(param_1 + 0x2d0) = iVar1;
      if (*(int *)(param_1 + 0x2dc) < iVar1) {
        *(int *)(param_1 + 0x2d0) = *(int *)(param_1 + 0x2dc);
      }
      *(undefined4 *)(param_1 + 0x2e4) = 1;
    }
  }
  FUN_004c3ad0(param_1,param_2,param_3);
  return;
}



// ==== @0x51ccf4 -> FUN_0051ccf4 @0x51ccf4 ====

void FUN_0051ccf4(int param_1,undefined4 param_2)

{
  undefined4 uVar1;
  
  uVar1 = FUN_0051c080(param_1);
  FUN_0049d5b8(param_2,uVar1);
  FUN_0049d608(param_2,*(undefined4 *)(param_1 + 0x264));
  FUN_0049d608(param_2,*(undefined4 *)(param_1 + 0x268));
  uVar1 = FUN_0051c0dc(param_1);
  FUN_0049d5b8(param_2,uVar1);
  FUN_0049d5b8(param_2,*(undefined4 *)(param_1 + 0x270));
  uVar1 = FUN_0051c178(param_1);
  FUN_0049d5b8(param_2,uVar1);
  uVar1 = FUN_0051c1c4(param_1);
  FUN_0049d5b8(param_2,uVar1);
  uVar1 = FUN_0051c244(param_1);
  FUN_0049d5b8(param_2,uVar1);
  uVar1 = FUN_0051c290(param_1);
  FUN_0049d5b8(param_2,uVar1);
  uVar1 = FUN_0051c3c0(param_1);
  FUN_0049d5b8(param_2,uVar1);
  uVar1 = FUN_0051c470(param_1);
  FUN_0049d5b8(param_2,uVar1);
  FUN_0049d5b8(param_2,*(undefined4 *)(param_1 + 0x28c));
  uVar1 = FUN_0051c564(param_1);
  FUN_0049d5b8(param_2,uVar1);
  uVar1 = FUN_0051c5b0(param_1);
  FUN_0049d5b8(param_2,uVar1);
  uVar1 = FUN_0051c630(param_1);
  FUN_0049d5b8(param_2,uVar1);
  uVar1 = FUN_0051c67c(param_1);
  FUN_0049d5b8(param_2,uVar1);
  uVar1 = FUN_0051c7ac(param_1);
  FUN_0049d5b8(param_2,uVar1);
  uVar1 = FUN_0051c85c(param_1);
  FUN_0049d5b8(param_2,uVar1);
  return;
}



