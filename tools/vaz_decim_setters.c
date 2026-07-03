// ==== @0x51dd44 -> FUN_0051dd44 @0x51dd44 ====

void FUN_0051dd44(int param_1,undefined4 param_2)

{
  byte bVar1;
  
  *(undefined4 *)(param_1 + 0x264) = param_2;
  bVar1 = 0x18 - (char)param_2;
  *(uint *)(param_1 + 0x270) = (0xffffffffU >> (bVar1 & 0x1f)) << (bVar1 & 0x1f);
  *(uint *)(param_1 + 0x274) = (1 << (bVar1 & 0x1f)) - 1U >> 1;
  return;
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



// ==== @0x51dd14 -> FUN_0051dd14 @0x51dd14 ====

void FUN_0051dd14(int param_1,int param_2)

{
  *(int *)(param_1 + 0x260) = param_2;
  *(int *)(param_1 + 0x26c) = ((param_2 + 1) * 0x2ee00) / **(int **)(param_1 + 0x1c);
  return;
}



// ==== @0x51da50 -> FUN_0051da50 @0x51da50 ====
// decompile failed

// ==== @0x51da9b -> FAIL ====

// ==== @0x51dabc -> FAIL ====

// ==== @0x51dd8c -> FUN_0051dd8c @0x51dd8c ====

void FUN_0051dd8c(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  
  uVar1 = FUN_0049d5cc(param_2);
  FUN_0051dd14(param_1,uVar1);
  uVar1 = FUN_0049d5cc(param_2);
  FUN_0051dd44(param_1,uVar1);
  return;
}



