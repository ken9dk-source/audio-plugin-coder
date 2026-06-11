// ===== FUN_004dbd7c @ 0x4dbd7c  (req 0x4dbd7c) =====

void FUN_004dbd7c(int param_1)

{
  while (*(int *)(param_1 + 0x4f4) <= *(int *)(param_1 + 0x4ec)) {
    FUN_004db9b8(param_1,*(undefined4 *)
                          (*(int *)(param_1 + 0x4f8) + 8 + *(int *)(param_1 + 0x4f0) * 4));
    *(int *)(param_1 + 0x4f0) = *(int *)(param_1 + 0x4f0) + 1;
    if (*(int *)(param_1 + 0x4f0) < **(int **)(param_1 + 0x4f8)) {
      *(undefined4 *)(param_1 + 0x4f4) =
           *(undefined4 *)((*(int **)(param_1 + 0x4f8))[*(int *)(param_1 + 0x4f0) + 2] + 8);
    }
    else {
      *(undefined4 *)(param_1 + 0x4f4) = 0x7fffffff;
    }
  }
  return;
}



// ===== FUN_004dfbf8 @ 0x4dfbf8  (req 0x4dfbf8) =====

void FUN_004dfbf8(int param_1,int param_2)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 0x2534 + param_2 * 4);
  *(undefined4 *)(iVar1 + 0x118) = *(undefined4 *)(iVar1 + 0xc4);
  *(undefined4 *)(iVar1 + 0x11c) = *(undefined4 *)(iVar1 + 0xc4);
  *(undefined4 *)(iVar1 + 0x120) = *(undefined4 *)(iVar1 + 0xc4);
  *(undefined4 *)(iVar1 + 0x124) = *(undefined4 *)(iVar1 + 0xc4);
  *(undefined4 *)(iVar1 + 0x130) = *(undefined4 *)(iVar1 + 0x13c);
  *(undefined4 *)(iVar1 + 300) = 0;
  return;
}



// ===== FUN_004a073c @ 0x4a073c  (req 0x4a073c) =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004a073c(int param_1,int param_2)

{
  int iVar1;
  
  *(int *)(param_1 + 4) = param_2;
  iVar1 = (*(int *)(param_1 + 0x78) * 0x3c00) / (param_2 * 0x18);
  *(int *)(param_1 + 8) = iVar1;
  *(float *)(param_1 + 0xc) = _DAT_004a0774 / (float)iVar1;
  return;
}



// ===== FUN_004a0a68 @ 0x4a0a68  (req 0x4a0a68) =====

undefined4 FUN_004a0a68(int param_1)

{
  if (*(char *)(param_1 + 0x7d) != '\0') {
    return *(undefined4 *)(param_1 + 0x68);
  }
  return *(undefined4 *)(param_1 + 4);
}



// ===== FUN_004a0864 @ 0x4a0864  (req 0x4a0864) =====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004a0864(int param_1,int param_2,undefined4 param_3)

{
  undefined4 *puVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  
  if (((*(byte *)(param_1 + 100) & 2) != 0) == (bool)*(char *)(param_1 + 0x7c)) {
    *(undefined4 *)(param_1 + 100) = 0;
  }
  else {
    *(undefined4 *)(param_1 + 100) = 1;
  }
  if (*(char *)(param_1 + 0x7c) != '\0') {
    *(int *)(param_1 + 100) = *(int *)(param_1 + 100) + 2;
    *(double *)(param_1 + 0x10) = (double)*(int *)(param_1 + 0x80);
    *(double *)(param_1 + 0x28) =
         (double)(_DAT_004a09f4 *
                 ((float10)*(int *)(param_1 + 0x84) +
                 (float10)*(int *)(param_1 + 0x88) * (float10)*(float *)(param_1 + 0xc)));
  }
  iVar3 = *(int *)(param_1 + 100) + 0x600;
  *(int *)(param_1 + 100) = iVar3;
  *(double *)(param_1 + 0x18) = (double)*(int *)(param_1 + 0x78);
  *(double *)(param_1 + 0x30) = (double)*(int *)(param_1 + 4);
  *(int *)(param_1 + 0x80) = *(int *)(param_1 + 0x80) + param_2;
  *(undefined4 *)(param_1 + 0x8098) = 0;
  iVar4 = 0;
  piVar2 = (int *)FUN_0049fbd0(*(undefined4 *)(param_1 + 0x74),iVar3,param_3,param_3);
  *(undefined4 *)(param_1 + 0x809c) = 0;
  if ((*(int *)(param_1 + 0x84) == 0) && (*(int *)(param_1 + 0x88) == 0)) {
    *(undefined4 *)(param_1 + 0x84) = 0xffffffff;
    *(undefined4 *)(param_1 + 0x88) = *(undefined4 *)(param_1 + 8);
  }
  if (-1 < param_2 + -1) {
    iVar3 = 0;
    do {
      *(int *)(param_1 + 0x88) = *(int *)(param_1 + 0x88) + 0x100;
      if (*(int *)(param_1 + 8) < *(int *)(param_1 + 0x88)) {
        if ((iVar4 < *piVar2) && (*(int *)(piVar2[iVar4 + 2] + 8) <= iVar3)) {
          FUN_004a0800(param_1,piVar2[iVar4 + 2]);
          iVar4 = iVar4 + 1;
        }
        *(int *)(param_1 + 0x88) = *(int *)(param_1 + 0x88) - *(int *)(param_1 + 8);
        *(int *)(param_1 + 0x84) = *(int *)(param_1 + 0x84) + 1;
        puVar1 = (undefined4 *)(param_1 + 0x94 + *(int *)(param_1 + 0x8098) * 0x20);
        *puVar1 = 1;
        puVar1[2] = iVar3;
        puVar1[4] = 0;
        *(undefined1 *)(puVar1 + 6) = 0xf8;
        *(undefined1 *)((int)puVar1 + 0x19) = 0;
        *(undefined1 *)((int)puVar1 + 0x1a) = 0;
        *(int *)(*(int *)(param_1 + 0x8094) + 8 + *(int *)(param_1 + 0x8098) * 4) =
             param_1 + 0x94 + *(int *)(param_1 + 0x8098) * 0x20;
        *(int *)(param_1 + 0x8098) = *(int *)(param_1 + 0x8098) + 1;
      }
      iVar3 = iVar3 + 1;
      param_2 = param_2 + -1;
    } while (param_2 != 0);
  }
  for (; iVar4 < *piVar2; iVar4 = iVar4 + 1) {
    FUN_004a0800(param_1,piVar2[iVar4 + 2]);
  }
  **(undefined4 **)(param_1 + 0x8094) = *(undefined4 *)(param_1 + 0x8098);
  return;
}



