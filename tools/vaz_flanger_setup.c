// ==== @0x520418 -> FUN_00520418 @0x520418 ====

void FUN_00520418(int param_1,undefined4 param_2,int param_3)

{
  int iVar1;
  undefined4 uVar2;
  
  *(int *)(param_1 + 0x2a4) =
       *(int *)(param_1 + 0x2a4) + (*(uint *)(param_1 + 0x2a4) >> 0x18) * 0x800000;
  *(int *)(param_1 + 0x2a4) = *(int *)(param_1 + 0x2a4) + param_3;
  if (*(int *)(param_1 + 0x270) != 0) {
    iVar1 = FUN_004a0a68(*(undefined4 *)(param_1 + 0x28));
    if ((iVar1 != *(int *)(param_1 + 0x2b0)) ||
       (*(int *)(param_1 + 0x274) != *(int *)(param_1 + 0x2b4))) {
      uVar2 = FUN_004a0a68(*(undefined4 *)(param_1 + 0x28));
      *(undefined4 *)(param_1 + 0x2b0) = uVar2;
      *(undefined4 *)(param_1 + 0x2b4) = *(undefined4 *)(param_1 + 0x274);
      uVar2 = FUN_00402bf4();
      *(undefined4 *)(param_1 + 0x290) = uVar2;
    }
  }
  FUN_004c3ad0(param_1,param_2,param_3);
  return;
}



// ==== @0x5203e4 -> FUN_005203e4 @0x5203e4 ====

void FUN_005203e4(void)

{
  int iVar1;
  uint extraout_EDX;
  
  iVar1 = FUN_00403830();
  if (*(int *)(iVar1 + 0x2a8) != 0) {
    FUN_00402784();
  }
  FUN_004c39e8(iVar1,extraout_EDX & 0xfffffffc);
  if ('\0' < (char)extraout_EDX) {
    FUN_00403818(iVar1);
  }
  return;
}



// ==== @0x5202cc -> FUN_005202cc @0x5202cc ====

int FUN_005202cc(int param_1,char param_2)

{
  char extraout_DL;
  char cVar1;
  undefined4 *in_FS_OFFSET;
  
  cVar1 = '\0';
  if (param_2 != '\0') {
    param_1 = FUN_004037c8();
    cVar1 = extraout_DL;
  }
  FUN_004c398c(param_1,0);
  *(undefined4 *)(param_1 + 0x58) = 4;
  *(undefined4 *)(param_1 + 0x5c) = 0xcb;
  *(undefined4 *)(param_1 + 0x274) = 0x60;
  *(undefined4 *)(param_1 + 0x284) = 0x80;
  FUN_005208f0(param_1,0xe1);
  FUN_004a2bac(*(undefined4 *)(param_1 + 0x14),&DAT_0052b87c);
  (**(code **)(**(int **)(param_1 + 0x14) + 8))();
  (**(code **)(**(int **)(param_1 + 0x14) + 8))();
  (**(code **)(**(int **)(param_1 + 0x14) + 8))();
  (**(code **)(**(int **)(param_1 + 0x14) + 8))();
  (**(code **)(**(int **)(param_1 + 0x14) + 8))();
  (**(code **)(**(int **)(param_1 + 0x14) + 8))();
  (**(code **)(**(int **)(param_1 + 0x14) + 8))();
  (**(code **)(**(int **)(param_1 + 0x14) + 8))();
  if (cVar1 != '\0') {
    FUN_00403820(param_1);
    *in_FS_OFFSET = FUN_005208f0;
  }
  return param_1;
}



