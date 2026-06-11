// ===== FUN_004db9b8 @ 0x4db9b8  (req 0x4db9b8) =====

void FUN_004db9b8(int *param_1,undefined4 *param_2)

{
  char *pcVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 local_2c [6];
  byte local_14;
  byte local_13;
  byte local_12;
  
  puVar5 = local_2c;
  for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar5 = *param_2;
    param_2 = param_2 + 1;
    puVar5 = puVar5 + 1;
  }
  FUN_004d058c(param_1[0x96d],local_2c);
  local_14 = local_14 & 0xf0;
  if (local_14 < 0xc1) {
    if (local_14 == 0xc0) {
      PostMessageA((HWND)param_1[0x142],0x469,(uint)local_13,0);
    }
    else if (local_14 == 0x80) {
      if ((char)param_1[0x107] == '\0') {
        if (*(char *)(param_1[0x96d] + 0x275c) != '\0') {
          FUN_004d1418(param_1[0x96d],local_13);
        }
        if (*(char *)(param_1[0x96d] + 0x275d) == '\0') {
          FUN_004db698(param_1,local_13);
        }
      }
      else {
        *(undefined1 *)((int)param_1 + local_13 + 0x391) = 1;
      }
    }
    else if (local_14 == 0x90) {
      if (local_12 == 0) {
        if ((char)param_1[0x107] == '\0') {
          if (*(char *)(param_1[0x96d] + 0x275c) != '\0') {
            FUN_004d1418(param_1[0x96d],local_13);
          }
          if (*(char *)(param_1[0x96d] + 0x275d) == '\0') {
            FUN_004db698(param_1,local_13);
          }
        }
        else {
          *(undefined1 *)((int)param_1 + local_13 + 0x391) = 1;
        }
      }
      else {
        if ((uint)local_13 != *(uint *)(PTR_DAT_0052bbd8 + 4)) {
          if (*(char *)(param_1[0x96d] + 0x275c) != '\0') {
            FUN_004d1234(param_1[0x96d],local_13,local_12);
          }
          if (*(char *)(param_1[0x96d] + 0x275d) == '\0') {
            FUN_004db3a8(param_1,local_13,local_12);
            if (*(char *)((int)param_1 + local_13 + 0x391) != '\0') {
              pcVar1 = (char *)((int)param_1 + local_13 + 0x311);
              *pcVar1 = *pcVar1 + -1;
            }
          }
        }
        *(undefined1 *)((int)param_1 + local_13 + 0x391) = 0;
      }
    }
    else if (local_14 == 0xb0) {
      iVar2 = param_1[0xc];
      if (iVar2 != -1) {
        if ((&DAT_00543a28)[iVar2] != -1) {
          (&DAT_00543ba8)[(&DAT_00543a28)[param_1[0xc]]] = 0xffffffff;
        }
        uVar3 = (uint)local_13;
        if ((&DAT_00543ba8)[uVar3] != -1) {
          (&DAT_00543a28)[(&DAT_00543ba8)[uVar3]] = 0xffffffff;
        }
        (&DAT_00543ba8)[uVar3] = iVar2;
        (&DAT_00543a28)[iVar2] = uVar3;
        param_1[0xc] = -1;
        FUN_004a467c(param_1[2],0x478,0,0);
      }
      if (local_13 == DAT_00543da8) {
        iVar2 = param_1[0x1c];
        if (-1 < iVar2 + -1) {
          iVar4 = 0;
          do {
            *(uint *)(param_1[iVar4 + 0x94d] + 0x4c) = (uint)local_12 * 0x20000 + -0x7fffff;
            iVar4 = iVar4 + 1;
            iVar2 = iVar2 + -1;
          } while (iVar2 != 0);
        }
      }
      else if (local_13 == DAT_00543da9) {
        iVar2 = param_1[0x1c];
        if (-1 < iVar2 + -1) {
          iVar4 = 0;
          do {
            *(uint *)(param_1[iVar4 + 0x94d] + 0x50) = (uint)local_12 * 0x20000 + -0x7fffff;
            iVar4 = iVar4 + 1;
            iVar2 = iVar2 + -1;
          } while (iVar2 != 0);
        }
      }
      else if (local_13 == 0x40) {
        FUN_004db8d4(param_1,0x3f < local_12);
      }
      else if ((local_13 == 0x7b) && (PTR_DAT_0052bbd8[1] != '\x02')) {
        (**(code **)(*param_1 + 0x38))();
      }
      if ((&DAT_00543ba8)[local_13] != -1) {
        FUN_004a2bd4(param_1[5],(&DAT_00543ba8)[local_13],local_12);
      }
    }
  }
  else if (local_14 == 0xd0) {
    iVar2 = param_1[0x1c];
    if (-1 < iVar2 + -1) {
      iVar4 = 0;
      do {
        *(uint *)(param_1[iVar4 + 0x94d] + 0x48) = (uint)local_13 * 0x20000 + -0x800000;
        iVar4 = iVar4 + 1;
        iVar2 = iVar2 + -1;
      } while (iVar2 != 0);
    }
  }
  else if (local_14 == 0xe0) {
    FUN_004db958(param_1,local_13,local_12);
  }
  return;
}



// ===== FUN_004a0864 @ 0x4a0864  (req 0x4a0900) =====

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



// no function contains 0x4a0c00
// ===== FUN_004a0db8 @ 0x4a0db8  (req 0x4a0e00) =====

undefined4 FUN_004a0db8(int param_1,uint param_2)

{
  undefined4 uVar1;
  
  if (*(char *)(param_1 + 0x7d) != '\0') {
    if (((*(uint *)(param_1 + 0x80c0) | param_2) != *(uint *)(param_1 + 0x80c0)) &&
       (*(uint *)(param_1 + 0x80c0) = *(uint *)(param_1 + 0x80c0) | param_2,
       *(short *)(param_1 + 0x80aa) != 0)) {
      uVar1 = (**(code **)(param_1 + 0x80a8))
                        (*(undefined4 *)(param_1 + 0x80ac),*(undefined4 *)(param_1 + 0x80c0));
      *(undefined4 *)(param_1 + 0x80bc) = uVar1;
    }
    return *(undefined4 *)(param_1 + 0x80bc);
  }
  uVar1 = FUN_004a0a00(param_1);
  return uVar1;
}



// no function contains 0x4a0700
