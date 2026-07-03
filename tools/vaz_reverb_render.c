// ==== forced @0x522400 -> FUN_005223e4 @0x5223e4 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_005223e4(void)

{
  undefined4 *in_FS_OFFSET;
  undefined4 uStack_10;
  undefined1 *puStack_c;
  undefined1 *puStack_8;
  
  puStack_8 = &stack0xfffffffc;
  puStack_c = &LAB_00522430;
  uStack_10 = *in_FS_OFFSET;
  *in_FS_OFFSET = &uStack_10;
  _DAT_006f8e70 = _DAT_006f8e70 + 1;
  if (_DAT_006f8e70 == 0) {
    FUN_00404d70(&DAT_0052b9e8,PTR_DAT_004bec1c);
    FUN_00404dbc(&DAT_0052b934,PTR_DAT_004a2914,9);
  }
  *in_FS_OFFSET = uStack_10;
  return;
}



// ==== forced @0x522700 -> FUN_005223e4 @0x5223e4 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_005223e4(void)

{
  undefined4 *in_FS_OFFSET;
  undefined4 uStack_10;
  undefined1 *puStack_c;
  undefined1 *puStack_8;
  
  puStack_8 = &stack0xfffffffc;
  puStack_c = &LAB_00522430;
  uStack_10 = *in_FS_OFFSET;
  *in_FS_OFFSET = &uStack_10;
  _DAT_006f8e70 = _DAT_006f8e70 + 1;
  if (_DAT_006f8e70 == 0) {
    FUN_00404d70(&DAT_0052b9e8,PTR_DAT_004bec1c);
    FUN_00404dbc(&DAT_0052b934,PTR_DAT_004a2914,9);
  }
  *in_FS_OFFSET = uStack_10;
  return;
}



// ==== forced @0x522a00 -> FUN_005228a4 @0x5228a4 ====

/* WARNING: Restarted to delay deadcode elimination for space: stack */

void FUN_005228a4(int param_1,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  
  iVar6 = 0;
  do {
    uVar4 = *(int *)(param_1 + 0x26c) + 1;
    uVar5 = uVar4 & 0xfff;
    *(uint *)(param_1 + 0x26c) = uVar5;
    iVar1 = *(int *)(param_2 + iVar6 * 8);
    iVar2 = *(int *)(param_2 + 4 + iVar6 * 8);
    iVar3 = iVar1 + iVar2 >> 6;
    iVar7 = (int)((ulonglong)
                  ((longlong)(*(int *)(param_1 + 0x278 + uVar5 * 4) * 2) *
                  (longlong)*(int *)(param_1 + 0x274)) >> 0x20);
    *(int *)(param_1 + 0x278 + (*(int *)(param_1 + 0x270) + uVar5 & 0xfff) * 4) = iVar7 + iVar3;
    iVar8 = (int)((ulonglong)
                  ((longlong)(*(int *)(param_1 + 0x4280 + uVar5 * 4) * 2) *
                  (longlong)*(int *)(param_1 + 0x427c)) >> 0x20);
    *(int *)(param_1 + 0x4280 + (*(int *)(param_1 + 0x4278) + uVar5 & 0xfff) * 4) = iVar8 + iVar3;
    iVar9 = (int)((ulonglong)
                  ((longlong)(*(int *)(param_1 + 0x8288 + uVar5 * 4) * 2) *
                  (longlong)*(int *)(param_1 + 0x8284)) >> 0x20);
    *(int *)(param_1 + 0x8288 + (*(int *)(param_1 + 0x8280) + uVar5 & 0xfff) * 4) = iVar9 + iVar3;
    iVar10 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0xc290 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0xc28c)) >> 0x20);
    *(int *)(param_1 + 0xc290 + (*(int *)(param_1 + 0xc288) + uVar5 & 0xfff) * 4) = iVar10 + iVar3;
    iVar11 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0x10298 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0x10294)) >> 0x20);
    *(int *)(param_1 + 0x10298 + (*(int *)(param_1 + 0x10290) + uVar5 & 0xfff) * 4) = iVar11 + iVar3
    ;
    iVar12 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0x142a0 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0x1429c)) >> 0x20);
    *(int *)(param_1 + 0x142a0 + (*(int *)(param_1 + 0x14298) + uVar5 & 0xfff) * 4) = iVar12 + iVar3
    ;
    iVar13 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0x182a8 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0x182a4)) >> 0x20);
    *(int *)(param_1 + 0x182a8 + (*(int *)(param_1 + 0x182a0) + uVar5 & 0xfff) * 4) = iVar13 + iVar3
    ;
    iVar14 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0x1c2b0 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0x1c2ac)) >> 0x20);
    iVar9 = iVar7 * 2 + iVar8 + iVar9 * 4 + iVar10 * 2 + iVar11 * 3 + iVar12 * 4 + iVar14 * 2;
    *(int *)(param_1 + 0x1c2b0 + (*(int *)(param_1 + 0x1c2a8) + uVar5 & 0xfff) * 4) = iVar14 + iVar3
    ;
    iVar12 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0x202b8 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0x202b4)) >> 0x20);
    iVar7 = iVar7 * 2 + iVar8 * 2 + iVar8 + iVar10 * 2 + iVar11 + iVar13 * 4 + iVar14 * 2 +
            iVar12 * 4;
    *(int *)(param_1 + 0x202b8 + (*(int *)(param_1 + 0x202b0) + uVar5 & 0xfff) * 4) = iVar12 + iVar3
    ;
    uVar4 = uVar4 & 0x3ff;
    iVar3 = *(int *)(param_1 + 0x282c4 + uVar4 * 4);
    iVar8 = iVar3 - iVar9;
    *(int *)(param_1 + 0x282c4 + (*(int *)(param_1 + 0x282c0) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar3 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar9;
    iVar9 = *(int *)(param_1 + 0x292c8 + uVar4 * 4);
    iVar3 = iVar9 - iVar8;
    *(int *)(param_1 + 0x292c8 + (*(int *)(param_1 + 0x292c4) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar9 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar8;
    iVar9 = *(int *)(param_1 + 0x2a2cc + uVar4 * 4);
    iVar8 = iVar9 - iVar3;
    *(int *)(param_1 + 0x2a2cc + (*(int *)(param_1 + 0x2a2c8) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar9 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar3;
    iVar9 = *(int *)(param_1 + 0x2b2d0 + uVar4 * 4);
    *(int *)(param_1 + 0x2b2d0 + (*(int *)(param_1 + 0x2b2cc) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar9 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar8;
    iVar3 = *(int *)(param_1 + 0x2c2d4 + uVar4 * 4);
    iVar10 = iVar3 - iVar7;
    *(int *)(param_1 + 0x2c2d4 + (*(int *)(param_1 + 0x2c2d0) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar3 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar7;
    iVar7 = *(int *)(param_1 + 0x2d2d8 + uVar4 * 4);
    iVar3 = iVar7 - iVar10;
    *(int *)(param_1 + 0x2d2d8 + (*(int *)(param_1 + 0x2d2d4) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar7 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar10;
    iVar7 = *(int *)(param_1 + 0x2e2dc + uVar4 * 4);
    iVar10 = iVar7 - iVar3;
    *(int *)(param_1 + 0x2e2dc + (*(int *)(param_1 + 0x2e2d8) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar7 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar3;
    iVar7 = *(int *)(param_1 + 0x2f2e0 + uVar4 * 4);
    *(int *)(param_1 + 0x2f2e0 + (*(int *)(param_1 + 0x2f2dc) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar7 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar10;
    iVar3 = (int)((ulonglong)
                  ((longlong)((iVar9 - iVar8) * 4) * (longlong)*(int *)(param_1 + 0x302e0)) >> 0x20)
            + (int)((ulonglong)
                    ((longlong)(*(int *)(param_1 + 0x302e8) << 2) *
                    (longlong)*(int *)(param_1 + 0x302e4)) >> 0x20);
    *(int *)(param_1 + 0x302e8) = iVar3;
    iVar7 = (int)((ulonglong)
                  ((longlong)((iVar7 - iVar10) * 4) * (longlong)*(int *)(param_1 + 0x302e0)) >> 0x20
                 ) + (int)((ulonglong)
                           ((longlong)(*(int *)(param_1 + 0x302ec) << 2) *
                           (longlong)*(int *)(param_1 + 0x302e4)) >> 0x20);
    *(int *)(param_1 + 0x302e8) = iVar7;
    iVar9 = *(int *)(param_1 + 0x268) << 0x17;
    *(int *)(param_2 + iVar6 * 8) =
         iVar1 + (int)((ulonglong)((longlong)((iVar3 - iVar1) * 2) * (longlong)iVar9) >> 0x20);
    *(int *)(param_2 + 4 + iVar6 * 8) =
         iVar2 + (int)((ulonglong)((longlong)((iVar7 - iVar2) * 2) * (longlong)iVar9) >> 0x20);
    iVar6 = iVar6 + 1;
  } while (iVar6 < param_3);
  return;
}



// ==== forced @0x522d00 -> FUN_005228a4 @0x5228a4 ====

/* WARNING: Restarted to delay deadcode elimination for space: stack */

void FUN_005228a4(int param_1,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  
  iVar6 = 0;
  do {
    uVar4 = *(int *)(param_1 + 0x26c) + 1;
    uVar5 = uVar4 & 0xfff;
    *(uint *)(param_1 + 0x26c) = uVar5;
    iVar1 = *(int *)(param_2 + iVar6 * 8);
    iVar2 = *(int *)(param_2 + 4 + iVar6 * 8);
    iVar3 = iVar1 + iVar2 >> 6;
    iVar7 = (int)((ulonglong)
                  ((longlong)(*(int *)(param_1 + 0x278 + uVar5 * 4) * 2) *
                  (longlong)*(int *)(param_1 + 0x274)) >> 0x20);
    *(int *)(param_1 + 0x278 + (*(int *)(param_1 + 0x270) + uVar5 & 0xfff) * 4) = iVar7 + iVar3;
    iVar8 = (int)((ulonglong)
                  ((longlong)(*(int *)(param_1 + 0x4280 + uVar5 * 4) * 2) *
                  (longlong)*(int *)(param_1 + 0x427c)) >> 0x20);
    *(int *)(param_1 + 0x4280 + (*(int *)(param_1 + 0x4278) + uVar5 & 0xfff) * 4) = iVar8 + iVar3;
    iVar9 = (int)((ulonglong)
                  ((longlong)(*(int *)(param_1 + 0x8288 + uVar5 * 4) * 2) *
                  (longlong)*(int *)(param_1 + 0x8284)) >> 0x20);
    *(int *)(param_1 + 0x8288 + (*(int *)(param_1 + 0x8280) + uVar5 & 0xfff) * 4) = iVar9 + iVar3;
    iVar10 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0xc290 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0xc28c)) >> 0x20);
    *(int *)(param_1 + 0xc290 + (*(int *)(param_1 + 0xc288) + uVar5 & 0xfff) * 4) = iVar10 + iVar3;
    iVar11 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0x10298 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0x10294)) >> 0x20);
    *(int *)(param_1 + 0x10298 + (*(int *)(param_1 + 0x10290) + uVar5 & 0xfff) * 4) = iVar11 + iVar3
    ;
    iVar12 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0x142a0 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0x1429c)) >> 0x20);
    *(int *)(param_1 + 0x142a0 + (*(int *)(param_1 + 0x14298) + uVar5 & 0xfff) * 4) = iVar12 + iVar3
    ;
    iVar13 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0x182a8 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0x182a4)) >> 0x20);
    *(int *)(param_1 + 0x182a8 + (*(int *)(param_1 + 0x182a0) + uVar5 & 0xfff) * 4) = iVar13 + iVar3
    ;
    iVar14 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0x1c2b0 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0x1c2ac)) >> 0x20);
    iVar9 = iVar7 * 2 + iVar8 + iVar9 * 4 + iVar10 * 2 + iVar11 * 3 + iVar12 * 4 + iVar14 * 2;
    *(int *)(param_1 + 0x1c2b0 + (*(int *)(param_1 + 0x1c2a8) + uVar5 & 0xfff) * 4) = iVar14 + iVar3
    ;
    iVar12 = (int)((ulonglong)
                   ((longlong)(*(int *)(param_1 + 0x202b8 + uVar5 * 4) * 2) *
                   (longlong)*(int *)(param_1 + 0x202b4)) >> 0x20);
    iVar7 = iVar7 * 2 + iVar8 * 2 + iVar8 + iVar10 * 2 + iVar11 + iVar13 * 4 + iVar14 * 2 +
            iVar12 * 4;
    *(int *)(param_1 + 0x202b8 + (*(int *)(param_1 + 0x202b0) + uVar5 & 0xfff) * 4) = iVar12 + iVar3
    ;
    uVar4 = uVar4 & 0x3ff;
    iVar3 = *(int *)(param_1 + 0x282c4 + uVar4 * 4);
    iVar8 = iVar3 - iVar9;
    *(int *)(param_1 + 0x282c4 + (*(int *)(param_1 + 0x282c0) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar3 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar9;
    iVar9 = *(int *)(param_1 + 0x292c8 + uVar4 * 4);
    iVar3 = iVar9 - iVar8;
    *(int *)(param_1 + 0x292c8 + (*(int *)(param_1 + 0x292c4) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar9 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar8;
    iVar9 = *(int *)(param_1 + 0x2a2cc + uVar4 * 4);
    iVar8 = iVar9 - iVar3;
    *(int *)(param_1 + 0x2a2cc + (*(int *)(param_1 + 0x2a2c8) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar9 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar3;
    iVar9 = *(int *)(param_1 + 0x2b2d0 + uVar4 * 4);
    *(int *)(param_1 + 0x2b2d0 + (*(int *)(param_1 + 0x2b2cc) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar9 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar8;
    iVar3 = *(int *)(param_1 + 0x2c2d4 + uVar4 * 4);
    iVar10 = iVar3 - iVar7;
    *(int *)(param_1 + 0x2c2d4 + (*(int *)(param_1 + 0x2c2d0) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar3 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar7;
    iVar7 = *(int *)(param_1 + 0x2d2d8 + uVar4 * 4);
    iVar3 = iVar7 - iVar10;
    *(int *)(param_1 + 0x2d2d8 + (*(int *)(param_1 + 0x2d2d4) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar7 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar10;
    iVar7 = *(int *)(param_1 + 0x2e2dc + uVar4 * 4);
    iVar10 = iVar7 - iVar3;
    *(int *)(param_1 + 0x2e2dc + (*(int *)(param_1 + 0x2e2d8) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar7 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar3;
    iVar7 = *(int *)(param_1 + 0x2f2e0 + uVar4 * 4);
    *(int *)(param_1 + 0x2f2e0 + (*(int *)(param_1 + 0x2f2dc) + uVar4 & 0x3ff) * 4) =
         (int)((ulonglong)((longlong)(iVar7 * 2) * (longlong)DAT_0052ba54) >> 0x20) + iVar10;
    iVar3 = (int)((ulonglong)
                  ((longlong)((iVar9 - iVar8) * 4) * (longlong)*(int *)(param_1 + 0x302e0)) >> 0x20)
            + (int)((ulonglong)
                    ((longlong)(*(int *)(param_1 + 0x302e8) << 2) *
                    (longlong)*(int *)(param_1 + 0x302e4)) >> 0x20);
    *(int *)(param_1 + 0x302e8) = iVar3;
    iVar7 = (int)((ulonglong)
                  ((longlong)((iVar7 - iVar10) * 4) * (longlong)*(int *)(param_1 + 0x302e0)) >> 0x20
                 ) + (int)((ulonglong)
                           ((longlong)(*(int *)(param_1 + 0x302ec) << 2) *
                           (longlong)*(int *)(param_1 + 0x302e4)) >> 0x20);
    *(int *)(param_1 + 0x302e8) = iVar7;
    iVar9 = *(int *)(param_1 + 0x268) << 0x17;
    *(int *)(param_2 + iVar6 * 8) =
         iVar1 + (int)((ulonglong)((longlong)((iVar3 - iVar1) * 2) * (longlong)iVar9) >> 0x20);
    *(int *)(param_2 + 4 + iVar6 * 8) =
         iVar2 + (int)((ulonglong)((longlong)((iVar7 - iVar2) * 2) * (longlong)iVar9) >> 0x20);
    iVar6 = iVar6 + 1;
  } while (iVar6 < param_3);
  return;
}



