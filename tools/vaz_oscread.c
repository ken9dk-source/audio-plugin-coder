// ==== forced @0x4d738f -> FUN_004d6c3c @0x4d6c3c ====

void FUN_004d6c3c(int param_1,int param_2,uint param_3)

{
  char cVar1;
  undefined4 uVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  undefined4 *in_FS_OFFSET;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined1 *puVar9;
  undefined1 *puVar10;
  undefined1 *puVar11;
  undefined1 *puVar12;
  undefined1 *puVar13;
  undefined1 *puVar14;
  undefined1 *puVar15;
  undefined1 *puVar16;
  undefined1 *puVar17;
  undefined4 uVar18;
  undefined1 *puVar19;
  undefined1 *puVar20;
  undefined1 *puVar21;
  undefined1 *puVar22;
  undefined1 *puVar23;
  undefined1 *puVar24;
  undefined1 *puVar25;
  undefined1 *puVar26;
  undefined1 *puVar27;
  undefined1 *puVar28;
  undefined1 *puVar29;
  undefined4 uStack_25c;
  undefined1 *puStack_258;
  undefined1 *puStack_254;
  undefined4 uStack_244;
  undefined4 uStack_240;
  undefined1 auStack_23c [514];
  char cStack_3a;
  char cStack_39;
  int iStack_38;
  char cStack_32;
  char cStack_31;
  int iStack_30;
  int iStack_2c;
  int iStack_28;
  undefined1 uStack_22;
  byte bStack_21;
  int iStack_20;
  undefined4 uStack_1c;
  int iStack_18;
  undefined1 uStack_11;
  uint uStack_10;
  int iStack_c;
  int iStack_8;
  
  puStack_254 = &stack0xfffffffc;
  puVar9 = &stack0xfffffffc;
  puVar10 = &stack0xfffffffc;
  puVar11 = &stack0xfffffffc;
  puVar12 = &stack0xfffffffc;
  puVar13 = &stack0xfffffffc;
  puVar14 = &stack0xfffffffc;
  puVar15 = &stack0xfffffffc;
  puVar16 = &stack0xfffffffc;
  puVar17 = &stack0xfffffffc;
  puVar19 = &stack0xfffffffc;
  puVar20 = &stack0xfffffffc;
  puVar21 = &stack0xfffffffc;
  puVar22 = &stack0xfffffffc;
  puVar23 = &stack0xfffffffc;
  puVar24 = &stack0xfffffffc;
  puVar25 = &stack0xfffffffc;
  puVar26 = &stack0xfffffffc;
  puVar27 = &stack0xfffffffc;
  puVar28 = &stack0xfffffffc;
  puVar29 = &stack0xfffffffc;
  uStack_244 = 0;
  uStack_240 = 0;
  iStack_28 = 0;
  puStack_258 = &LAB_004d7bc2;
  uStack_25c = *in_FS_OFFSET;
  *in_FS_OFFSET = &uStack_25c;
  uStack_11 = 0;
  iStack_20 = 0xcb;
  uStack_10 = param_3;
  iStack_8 = param_2;
  cVar1 = FUN_004d68cc(&iStack_20,&uStack_1c,param_3,&stack0xfffffffc);
  if (cVar1 != '\0') {
    cStack_39 = (**(code **)(param_1 + 0x58))(*(undefined4 *)(param_1 + 0x5c),param_1,1);
    iStack_c = 0xcb;
    cVar1 = FUN_004d6a80(&iStack_c,&iStack_18);
    if (cVar1 != '\0') {
      if (iStack_c < 0x67) {
        cStack_32 = '\x01';
      }
      else {
        cStack_32 = FUN_0049d5f4(iStack_8);
      }
      if (cStack_32 != '\0') {
        FUN_004de664(param_1,0xffffffff);
      }
      if (iStack_c < 0x6d) {
        if (cStack_32 == '\0') {
          iStack_38 = 1;
        }
        else {
          iStack_38 = 0x10;
        }
      }
      else {
        iStack_38 = FUN_0049d5cc(iStack_8);
      }
      if (iStack_c < 0xc9) {
        FUN_004dea9c(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004dea9c(param_1,uVar2);
      }
      if (iStack_c < 0xc9) {
        *(undefined4 *)(param_1 + 0x94) = 0x60;
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        *(undefined4 *)(param_1 + 0x94) = uVar2;
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004de804(param_1,uVar2);
      iVar3 = FUN_0049d5cc(iStack_8);
      if (iStack_c < 200) {
        if (iVar3 == 1) {
          iVar3 = 4;
        }
        else {
          iVar3 = 0;
        }
      }
      FUN_004de8a0(param_1,iVar3);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004de930(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004de8f8(param_1,uVar2);
      if (iStack_c < 0xc9) {
        FUN_004ded84(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004ded84(param_1,uVar2);
      }
      if (iStack_c < 0xc9) {
        *(undefined4 *)(param_1 + 0xe0) = 0x60;
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        *(undefined4 *)(param_1 + 0xe0) = uVar2;
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dead8(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004deb50(param_1,0);
        *(undefined4 *)(param_1 + 0xcc) = 0;
        FUN_004deb60(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004deb50(param_1,uVar2,puVar9);
        uVar2 = FUN_0049d668(iStack_8,param_1 + 0xcc);
        FUN_004deb60(param_1,uVar2);
      }
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004debf8(param_1,uVar2);
      if (iStack_c < 200) {
        cVar1 = FUN_0049d5f4(iStack_8);
        if (cVar1 == '\0') {
          FUN_004deba0(param_1,1);
        }
        else {
          FUN_004deba0(param_1,6);
        }
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004deba0(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dec30(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dedb8(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004dedd8(param_1,uVar2);
      if (iStack_c < 0x6b) {
        iVar3 = FUN_0049d5cc(iStack_8);
        iStack_2c = FUN_0049d5cc(iStack_8);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004deeac(param_1,uVar2);
        iStack_30 = FUN_0049d5cc(iStack_8);
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004def30(param_1,uVar2);
        cStack_31 = FUN_0049d5f4(iStack_8);
        if (cStack_31 == '\0') {
          FUN_004dee54(param_1,iVar3 + 0x3f);
        }
        else {
          FUN_004dee54(param_1,iVar3);
        }
        if (cStack_31 == '\0') {
          FUN_004dee74(param_1,iStack_2c + 0x94);
        }
        else {
          FUN_004dee74(param_1,iStack_2c + 0x55);
        }
        if (cStack_31 == '\0') {
          FUN_004deef8(param_1,iStack_30 + 0x94);
        }
        else {
          FUN_004deef8(param_1,iStack_30 + 0x55);
        }
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004dee54(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004dee74(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004deeac(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004deef8(param_1,uVar2);
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004def30(param_1,uVar2);
      }
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004def40(param_1,uVar2);
      if (iStack_c < 0x6b) {
        FUN_004def50(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004def50(param_1,uVar2);
      }
      if (iStack_c < 0xca) {
        FUN_004def60(param_1,1);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004def60(param_1,uVar2);
      }
      if (iStack_c < 0x6c) {
        iVar3 = FUN_0049d5cc(iStack_8);
        iStack_2c = FUN_0049d5cc(iStack_8);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df0ac(param_1,uVar2);
        iStack_30 = FUN_0049d5cc(iStack_8);
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004df130(param_1,uVar2);
        cStack_31 = FUN_0049d5f4(iStack_8);
        if (cStack_31 == '\0') {
          FUN_004df054(param_1,iVar3 + 0x3f);
        }
        else {
          FUN_004df054(param_1,iVar3);
        }
        if (cStack_31 == '\0') {
          FUN_004df074(param_1,iStack_2c + 0x94);
        }
        else {
          FUN_004df074(param_1,iStack_2c + 0x55);
        }
        if (cStack_31 == '\0') {
          FUN_004df0f8(param_1,iStack_30 + 0x94);
        }
        else {
          FUN_004df0f8(param_1,iStack_30 + 0x55);
        }
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df054(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df074(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df0ac(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df0f8(param_1,uVar2);
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004df130(param_1,uVar2);
      }
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004df140(param_1,uVar2);
      if (iStack_c < 0x6c) {
        FUN_004df150(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004df150(param_1,uVar2);
      }
      if (iStack_c < 0xca) {
        FUN_004df160(param_1,1);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004df160(param_1,uVar2);
      }
      if (iStack_c < 200) {
        FUN_004df174(param_1,0);
        *(undefined4 *)(param_1 + 0x178) = 0;
        FUN_004df188(param_1,0);
        FUN_004df1c8(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004df174(param_1,uVar2,puVar10);
        uVar2 = FUN_0049d668(iStack_8,param_1 + 0x178);
        FUN_004df188(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df1c8(param_1,uVar2);
      }
      uVar2 = FUN_004d6c18();
      FUN_004e0460(param_1,uVar2,puVar11);
      if (iStack_c < 200) {
        FUN_004e0470(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004e0470(param_1,uVar2);
      }
      uVar2 = FUN_004d6c18();
      FUN_004e0480(param_1,uVar2,puVar12);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x2c4);
      FUN_004e0494(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004e04d4(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004e04d4(param_1,uVar2,puVar13);
      }
      if (iStack_c < 200) {
        FUN_004e04d4(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004e04e4(param_1,uVar2,puVar14);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004df300(param_1,uVar2);
      iVar3 = FUN_0049d5cc(iStack_8);
      FUN_004df3c0(param_1,*(undefined4 *)(&DAT_0052a8b8 + iVar3 * 4));
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004df3d0(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004df430(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004df430(param_1,uVar2);
      }
      uVar2 = FUN_004d6c18();
      FUN_004df440(param_1,uVar2,puVar15);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x1bc);
      FUN_004df454(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004df494(param_1,uVar2,puVar16);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x1c4);
      FUN_004df4a8(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004df4e8(param_1,uVar2,puVar17);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x1cc);
      FUN_004df4fc(param_1,uVar2);
      if (iStack_c < 0x69) {
        cVar1 = FUN_0049d5f4(iStack_8);
        if (cVar1 == '\0') {
          bStack_21 = 1;
        }
        else {
          bStack_21 = 2;
        }
        uStack_22 = FUN_0049d5f4(iStack_8);
        FUN_0049d530(iStack_8,&iStack_28);
        if (iStack_28 == 0) {
          FUN_004df704(param_1);
        }
        else {
          FUN_00402e38(auStack_23c,0x200,0);
          FUN_004090e8(*(undefined4 *)(iStack_8 + 4),&uStack_240);
          uVar8 = 0;
          uVar4 = (uint)bStack_21;
          uVar7 = 0x4d742a;
          uVar2 = uStack_240;
          iVar3 = FUN_004df3b8(param_1);
          uStack_25c = CONCAT31((int3)((uint)iVar3 >> 8),iVar3 != 3);
          FUN_004baac4(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,iStack_28,uVar7,uVar4,uVar8,uVar2
                      );
          FUN_004df690(param_1,auStack_23c,uStack_22);
          FUN_004baa34(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,0,1);
        }
      }
      else {
        FUN_00402e38(auStack_23c,0x200,0);
        uVar18 = 0;
        uVar8 = 1;
        uVar7 = 1;
        uVar2 = 0x4d7379;
        iVar3 = FUN_004df3b8(param_1);
        uStack_25c = CONCAT31((int3)((uint)iVar3 >> 8),iVar3 != 3);
        FUN_004bad88(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,iStack_8,uVar2,uVar7,uVar8,uVar18);
        uVar2 = FUN_0049d5f4(iStack_8);
        FUN_004df690(param_1,auStack_23c,uVar2);
        FUN_004baa34(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,0,1);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004df838(param_1,uVar2);
      iVar3 = FUN_0049d5cc(iStack_8);
      FUN_004df8f8(param_1,*(undefined4 *)(&DAT_0052a8b8 + iVar3 * 4));
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004df908(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004df918(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004df990(param_1,uVar2,puVar19);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x1f8);
      FUN_004df9a4(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004df9e4(param_1,uVar2,puVar20);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x200);
      FUN_004df9f8(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004dfa38(param_1,uVar2,puVar21);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x208);
      FUN_004dfa4c(param_1,uVar2);
      if (iStack_c < 0x6a) {
        cVar1 = FUN_0049d5f4(iStack_8);
        if (cVar1 == '\0') {
          bStack_21 = 1;
        }
        else {
          bStack_21 = 2;
        }
        uStack_22 = FUN_0049d5f4(iStack_8);
        FUN_0049d530(iStack_8,&iStack_28);
        if (iStack_28 == 0) {
          FUN_004dfcb8(param_1);
        }
        else {
          FUN_00402e38(auStack_23c,0x200,0);
          FUN_004090e8(*(undefined4 *)(iStack_8 + 4),&uStack_244);
          uVar8 = 0;
          uVar4 = (uint)bStack_21;
          uVar7 = 0x4d7602;
          uVar2 = uStack_244;
          iVar3 = FUN_004df8f0(param_1);
          uStack_25c = CONCAT31((int3)((uint)iVar3 >> 8),iVar3 != 3);
          FUN_004baac4(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,iStack_28,uVar7,uVar4,uVar8,uVar2
                      );
          FUN_004dfc44(param_1,auStack_23c,uStack_22);
          FUN_004baa34(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,0,1);
        }
      }
      else {
        FUN_00402e38(auStack_23c,0x200,0);
        uVar18 = 0;
        uVar8 = 1;
        uVar7 = 1;
        uVar2 = 0x4d7551;
        iVar3 = FUN_004df8f0(param_1);
        uStack_25c = CONCAT31((int3)((uint)iVar3 >> 8),iVar3 != 3);
        FUN_004bad88(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,iStack_8,uVar2,uVar7,uVar8,uVar18);
        uVar2 = FUN_0049d5f4(iStack_8);
        FUN_004dfc44(param_1,auStack_23c,uVar2);
        FUN_004baa34(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,0,1);
      }
      if (iStack_c < 200) {
        FUN_004dfd2c(param_1,0);
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004dfd2c(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dfd5c(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004dfd4c(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004dfde4(param_1,0);
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004dfde4(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dfe28(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004dfe18(param_1,uVar2);
      if (iStack_c < 200) {
        iVar3 = FUN_0049d5cc(iStack_8);
        FUN_004dfeb0(param_1,*(undefined4 *)(&DAT_0052a918 + iVar3 * 4));
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004dfeb0(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dfef4(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004dfee4(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dffb0(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004e018c(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e01a4(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e01b4(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e01c4(param_1,uVar2);
      if (iStack_c < 200) {
        if (*(int *)(param_1 + 0x254) == 9) {
          uVar2 = FUN_004e01bc(param_1);
          FUN_004e01d4(param_1,uVar2);
          FUN_004e01c4(param_1,0);
        }
        else {
          FUN_004e01d4(param_1,0);
        }
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004e01d4(param_1,uVar2);
      }
      uVar2 = FUN_004d6c18();
      FUN_004e01e4(param_1,uVar2,puVar22);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x27c);
      FUN_004e01f8(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004e0238(param_1,uVar2,puVar23);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x284);
      FUN_004e024c(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004e028c(param_1,uVar2,puVar24);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x28c);
      FUN_004e02a0(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004e02e0(param_1,uVar2,puVar25);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x294);
      FUN_004e02f4(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004e0334(param_1,uVar2,puVar26);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x29c);
      FUN_004e0348(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004e0388(param_1,uVar2,puVar27);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x2a4);
      FUN_004e039c(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004e03e4(param_1,0);
        *(undefined4 *)(param_1 + 0x2ac) = 0;
        FUN_004e03f8(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004e03e4(param_1,uVar2,puVar28);
        uVar2 = FUN_0049d668(iStack_8,param_1 + 0x2ac);
        FUN_004e03f8(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e0430(param_1,uVar2);
      if (iStack_c < 0x65) {
        FUN_004e04f4(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004e04f4(param_1,uVar2,puVar29);
      }
      if (iStack_c < 0x65) {
        FUN_004e0504(param_1,0);
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004e0504(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e0530(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e05f0(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004e0600(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e0610(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004e095c(param_1,0x10);
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004e095c(param_1,uVar2);
      }
      iVar3 = FUN_0049d5cc(iStack_8);
      if ((iStack_c < 0xcb) && (1 < iStack_38)) {
        iVar5 = FUN_004e0954(param_1);
        iVar6 = iStack_38;
        if (iVar5 < iStack_38) {
          iVar6 = FUN_004e0954(param_1);
        }
        iVar3 = (iVar3 * 0x12) / iVar6;
        if (0xff < iVar3) {
          iVar3 = 0xff;
        }
      }
      *(int *)(param_1 + 0x2f4) = iVar3;
      if (199 < iStack_c) {
        iVar3 = FUN_0049d5cc(iStack_8);
      }
      if ((iStack_c < 0xcb) && (iVar3 = iVar3 * 3, 0xff < iVar3)) {
        iVar3 = 0xff;
      }
      *(int *)(param_1 + 0x2f0) = iVar3;
      uVar2 = FUN_004e0978(param_1);
      FUN_004e0990(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e09d8(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004e09b8(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004e09c8(param_1,uVar2);
      cStack_3a = '\0';
      if ((uStack_10 & 1) == 0) {
        cVar1 = FUN_0049d5f4(iStack_8);
        if (cVar1 != '\0') {
          FUN_004a48cc(*(undefined4 *)(*(int *)(param_1 + 0x25b4) + 0xc),iStack_8);
          if (0x65 < iStack_c) {
            FUN_004a4574(*(undefined4 *)(*(int *)(param_1 + 0x25b4) + 8),iStack_8);
          }
          (**(code **)(**(int **)(param_1 + 0x25b4) + 0xc))(*(int **)(param_1 + 0x25b4),iStack_8,0);
          cStack_3a = '\x01';
        }
      }
      else {
        cVar1 = FUN_0049d5f4(iStack_8);
        if (cVar1 != '\0') {
          FUN_0049d45c(iStack_8,iStack_18 + -9,0);
        }
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004d1600(*(undefined4 *)(param_1 + 0x25b4),uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004d1618(*(undefined4 *)(param_1 + 0x25b4),uVar2);
      cVar1 = FUN_0049d5f4(iStack_8);
      if (cVar1 == '\0') {
        if (cStack_3a == '\0') {
          FUN_004d14cc(*(undefined4 *)(param_1 + 0x25b4),0);
        }
      }
      else {
        FUN_004d14cc(*(undefined4 *)(param_1 + 0x25b4),3);
      }
      FUN_0049dc8c(iStack_8,iStack_18,"preset");
      FUN_004de5e0(param_1,iStack_38);
      if (cStack_32 == '\0') {
        FUN_004de664(param_1,0);
      }
    }
    FUN_004be828(param_1,iStack_8,0x67 < iStack_20,(uStack_10 & 2) != 0);
    FUN_0049dc8c(iStack_8,uStack_1c,"Patch");
    uStack_11 = 1;
    FUN_004a454c(*(undefined4 *)(param_1 + 8));
    if (cStack_39 != '\0') {
      (**(code **)(param_1 + 0x58))(*(undefined4 *)(param_1 + 0x5c),param_1,0);
    }
  }
  puVar9 = puStack_254;
  *in_FS_OFFSET = uStack_25c;
  puStack_254 = &LAB_004d7bc9;
  puStack_258 = (undefined1 *)0x4d7bb9;
  FUN_004042a0(&uStack_244,2,puVar9);
  puStack_258 = (undefined1 *)0x4d7bc1;
  FUN_0040427c(&iStack_28);
  return;
}



// ==== forced @0x4d7567 -> FUN_004d6c3c @0x4d6c3c ====

void FUN_004d6c3c(int param_1,int param_2,uint param_3)

{
  char cVar1;
  undefined4 uVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  undefined4 *in_FS_OFFSET;
  undefined4 uVar7;
  undefined4 uVar8;
  undefined1 *puVar9;
  undefined1 *puVar10;
  undefined1 *puVar11;
  undefined1 *puVar12;
  undefined1 *puVar13;
  undefined1 *puVar14;
  undefined1 *puVar15;
  undefined1 *puVar16;
  undefined1 *puVar17;
  undefined4 uVar18;
  undefined1 *puVar19;
  undefined1 *puVar20;
  undefined1 *puVar21;
  undefined1 *puVar22;
  undefined1 *puVar23;
  undefined1 *puVar24;
  undefined1 *puVar25;
  undefined1 *puVar26;
  undefined1 *puVar27;
  undefined1 *puVar28;
  undefined1 *puVar29;
  undefined4 uStack_25c;
  undefined1 *puStack_258;
  undefined1 *puStack_254;
  undefined4 uStack_244;
  undefined4 uStack_240;
  undefined1 auStack_23c [514];
  char cStack_3a;
  char cStack_39;
  int iStack_38;
  char cStack_32;
  char cStack_31;
  int iStack_30;
  int iStack_2c;
  int iStack_28;
  undefined1 uStack_22;
  byte bStack_21;
  int iStack_20;
  undefined4 uStack_1c;
  int iStack_18;
  undefined1 uStack_11;
  uint uStack_10;
  int iStack_c;
  int iStack_8;
  
  puStack_254 = &stack0xfffffffc;
  puVar9 = &stack0xfffffffc;
  puVar10 = &stack0xfffffffc;
  puVar11 = &stack0xfffffffc;
  puVar12 = &stack0xfffffffc;
  puVar13 = &stack0xfffffffc;
  puVar14 = &stack0xfffffffc;
  puVar15 = &stack0xfffffffc;
  puVar16 = &stack0xfffffffc;
  puVar17 = &stack0xfffffffc;
  puVar19 = &stack0xfffffffc;
  puVar20 = &stack0xfffffffc;
  puVar21 = &stack0xfffffffc;
  puVar22 = &stack0xfffffffc;
  puVar23 = &stack0xfffffffc;
  puVar24 = &stack0xfffffffc;
  puVar25 = &stack0xfffffffc;
  puVar26 = &stack0xfffffffc;
  puVar27 = &stack0xfffffffc;
  puVar28 = &stack0xfffffffc;
  puVar29 = &stack0xfffffffc;
  uStack_244 = 0;
  uStack_240 = 0;
  iStack_28 = 0;
  puStack_258 = &LAB_004d7bc2;
  uStack_25c = *in_FS_OFFSET;
  *in_FS_OFFSET = &uStack_25c;
  uStack_11 = 0;
  iStack_20 = 0xcb;
  uStack_10 = param_3;
  iStack_8 = param_2;
  cVar1 = FUN_004d68cc(&iStack_20,&uStack_1c,param_3,&stack0xfffffffc);
  if (cVar1 != '\0') {
    cStack_39 = (**(code **)(param_1 + 0x58))(*(undefined4 *)(param_1 + 0x5c),param_1,1);
    iStack_c = 0xcb;
    cVar1 = FUN_004d6a80(&iStack_c,&iStack_18);
    if (cVar1 != '\0') {
      if (iStack_c < 0x67) {
        cStack_32 = '\x01';
      }
      else {
        cStack_32 = FUN_0049d5f4(iStack_8);
      }
      if (cStack_32 != '\0') {
        FUN_004de664(param_1,0xffffffff);
      }
      if (iStack_c < 0x6d) {
        if (cStack_32 == '\0') {
          iStack_38 = 1;
        }
        else {
          iStack_38 = 0x10;
        }
      }
      else {
        iStack_38 = FUN_0049d5cc(iStack_8);
      }
      if (iStack_c < 0xc9) {
        FUN_004dea9c(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004dea9c(param_1,uVar2);
      }
      if (iStack_c < 0xc9) {
        *(undefined4 *)(param_1 + 0x94) = 0x60;
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        *(undefined4 *)(param_1 + 0x94) = uVar2;
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004de804(param_1,uVar2);
      iVar3 = FUN_0049d5cc(iStack_8);
      if (iStack_c < 200) {
        if (iVar3 == 1) {
          iVar3 = 4;
        }
        else {
          iVar3 = 0;
        }
      }
      FUN_004de8a0(param_1,iVar3);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004de930(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004de8f8(param_1,uVar2);
      if (iStack_c < 0xc9) {
        FUN_004ded84(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004ded84(param_1,uVar2);
      }
      if (iStack_c < 0xc9) {
        *(undefined4 *)(param_1 + 0xe0) = 0x60;
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        *(undefined4 *)(param_1 + 0xe0) = uVar2;
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dead8(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004deb50(param_1,0);
        *(undefined4 *)(param_1 + 0xcc) = 0;
        FUN_004deb60(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004deb50(param_1,uVar2,puVar9);
        uVar2 = FUN_0049d668(iStack_8,param_1 + 0xcc);
        FUN_004deb60(param_1,uVar2);
      }
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004debf8(param_1,uVar2);
      if (iStack_c < 200) {
        cVar1 = FUN_0049d5f4(iStack_8);
        if (cVar1 == '\0') {
          FUN_004deba0(param_1,1);
        }
        else {
          FUN_004deba0(param_1,6);
        }
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004deba0(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dec30(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dedb8(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004dedd8(param_1,uVar2);
      if (iStack_c < 0x6b) {
        iVar3 = FUN_0049d5cc(iStack_8);
        iStack_2c = FUN_0049d5cc(iStack_8);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004deeac(param_1,uVar2);
        iStack_30 = FUN_0049d5cc(iStack_8);
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004def30(param_1,uVar2);
        cStack_31 = FUN_0049d5f4(iStack_8);
        if (cStack_31 == '\0') {
          FUN_004dee54(param_1,iVar3 + 0x3f);
        }
        else {
          FUN_004dee54(param_1,iVar3);
        }
        if (cStack_31 == '\0') {
          FUN_004dee74(param_1,iStack_2c + 0x94);
        }
        else {
          FUN_004dee74(param_1,iStack_2c + 0x55);
        }
        if (cStack_31 == '\0') {
          FUN_004deef8(param_1,iStack_30 + 0x94);
        }
        else {
          FUN_004deef8(param_1,iStack_30 + 0x55);
        }
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004dee54(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004dee74(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004deeac(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004deef8(param_1,uVar2);
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004def30(param_1,uVar2);
      }
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004def40(param_1,uVar2);
      if (iStack_c < 0x6b) {
        FUN_004def50(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004def50(param_1,uVar2);
      }
      if (iStack_c < 0xca) {
        FUN_004def60(param_1,1);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004def60(param_1,uVar2);
      }
      if (iStack_c < 0x6c) {
        iVar3 = FUN_0049d5cc(iStack_8);
        iStack_2c = FUN_0049d5cc(iStack_8);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df0ac(param_1,uVar2);
        iStack_30 = FUN_0049d5cc(iStack_8);
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004df130(param_1,uVar2);
        cStack_31 = FUN_0049d5f4(iStack_8);
        if (cStack_31 == '\0') {
          FUN_004df054(param_1,iVar3 + 0x3f);
        }
        else {
          FUN_004df054(param_1,iVar3);
        }
        if (cStack_31 == '\0') {
          FUN_004df074(param_1,iStack_2c + 0x94);
        }
        else {
          FUN_004df074(param_1,iStack_2c + 0x55);
        }
        if (cStack_31 == '\0') {
          FUN_004df0f8(param_1,iStack_30 + 0x94);
        }
        else {
          FUN_004df0f8(param_1,iStack_30 + 0x55);
        }
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df054(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df074(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df0ac(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df0f8(param_1,uVar2);
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004df130(param_1,uVar2);
      }
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004df140(param_1,uVar2);
      if (iStack_c < 0x6c) {
        FUN_004df150(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004df150(param_1,uVar2);
      }
      if (iStack_c < 0xca) {
        FUN_004df160(param_1,1);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004df160(param_1,uVar2);
      }
      if (iStack_c < 200) {
        FUN_004df174(param_1,0);
        *(undefined4 *)(param_1 + 0x178) = 0;
        FUN_004df188(param_1,0);
        FUN_004df1c8(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004df174(param_1,uVar2,puVar10);
        uVar2 = FUN_0049d668(iStack_8,param_1 + 0x178);
        FUN_004df188(param_1,uVar2);
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004df1c8(param_1,uVar2);
      }
      uVar2 = FUN_004d6c18();
      FUN_004e0460(param_1,uVar2,puVar11);
      if (iStack_c < 200) {
        FUN_004e0470(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004e0470(param_1,uVar2);
      }
      uVar2 = FUN_004d6c18();
      FUN_004e0480(param_1,uVar2,puVar12);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x2c4);
      FUN_004e0494(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004e04d4(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004e04d4(param_1,uVar2,puVar13);
      }
      if (iStack_c < 200) {
        FUN_004e04d4(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004e04e4(param_1,uVar2,puVar14);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004df300(param_1,uVar2);
      iVar3 = FUN_0049d5cc(iStack_8);
      FUN_004df3c0(param_1,*(undefined4 *)(&DAT_0052a8b8 + iVar3 * 4));
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004df3d0(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004df430(param_1,0);
      }
      else {
        uVar2 = FUN_0049d620(iStack_8);
        FUN_004df430(param_1,uVar2);
      }
      uVar2 = FUN_004d6c18();
      FUN_004df440(param_1,uVar2,puVar15);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x1bc);
      FUN_004df454(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004df494(param_1,uVar2,puVar16);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x1c4);
      FUN_004df4a8(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004df4e8(param_1,uVar2,puVar17);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x1cc);
      FUN_004df4fc(param_1,uVar2);
      if (iStack_c < 0x69) {
        cVar1 = FUN_0049d5f4(iStack_8);
        if (cVar1 == '\0') {
          bStack_21 = 1;
        }
        else {
          bStack_21 = 2;
        }
        uStack_22 = FUN_0049d5f4(iStack_8);
        FUN_0049d530(iStack_8,&iStack_28);
        if (iStack_28 == 0) {
          FUN_004df704(param_1);
        }
        else {
          FUN_00402e38(auStack_23c,0x200,0);
          FUN_004090e8(*(undefined4 *)(iStack_8 + 4),&uStack_240);
          uVar8 = 0;
          uVar4 = (uint)bStack_21;
          uVar7 = 0x4d742a;
          uVar2 = uStack_240;
          iVar3 = FUN_004df3b8(param_1);
          uStack_25c = CONCAT31((int3)((uint)iVar3 >> 8),iVar3 != 3);
          FUN_004baac4(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,iStack_28,uVar7,uVar4,uVar8,uVar2
                      );
          FUN_004df690(param_1,auStack_23c,uStack_22);
          FUN_004baa34(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,0,1);
        }
      }
      else {
        FUN_00402e38(auStack_23c,0x200,0);
        uVar18 = 0;
        uVar8 = 1;
        uVar7 = 1;
        uVar2 = 0x4d7379;
        iVar3 = FUN_004df3b8(param_1);
        uStack_25c = CONCAT31((int3)((uint)iVar3 >> 8),iVar3 != 3);
        FUN_004bad88(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,iStack_8,uVar2,uVar7,uVar8,uVar18);
        uVar2 = FUN_0049d5f4(iStack_8);
        FUN_004df690(param_1,auStack_23c,uVar2);
        FUN_004baa34(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,0,1);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004df838(param_1,uVar2);
      iVar3 = FUN_0049d5cc(iStack_8);
      FUN_004df8f8(param_1,*(undefined4 *)(&DAT_0052a8b8 + iVar3 * 4));
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004df908(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004df918(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004df990(param_1,uVar2,puVar19);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x1f8);
      FUN_004df9a4(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004df9e4(param_1,uVar2,puVar20);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x200);
      FUN_004df9f8(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004dfa38(param_1,uVar2,puVar21);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x208);
      FUN_004dfa4c(param_1,uVar2);
      if (iStack_c < 0x6a) {
        cVar1 = FUN_0049d5f4(iStack_8);
        if (cVar1 == '\0') {
          bStack_21 = 1;
        }
        else {
          bStack_21 = 2;
        }
        uStack_22 = FUN_0049d5f4(iStack_8);
        FUN_0049d530(iStack_8,&iStack_28);
        if (iStack_28 == 0) {
          FUN_004dfcb8(param_1);
        }
        else {
          FUN_00402e38(auStack_23c,0x200,0);
          FUN_004090e8(*(undefined4 *)(iStack_8 + 4),&uStack_244);
          uVar8 = 0;
          uVar4 = (uint)bStack_21;
          uVar7 = 0x4d7602;
          uVar2 = uStack_244;
          iVar3 = FUN_004df8f0(param_1);
          uStack_25c = CONCAT31((int3)((uint)iVar3 >> 8),iVar3 != 3);
          FUN_004baac4(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,iStack_28,uVar7,uVar4,uVar8,uVar2
                      );
          FUN_004dfc44(param_1,auStack_23c,uStack_22);
          FUN_004baa34(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,0,1);
        }
      }
      else {
        FUN_00402e38(auStack_23c,0x200,0);
        uVar18 = 0;
        uVar8 = 1;
        uVar7 = 1;
        uVar2 = 0x4d7551;
        iVar3 = FUN_004df8f0(param_1);
        uStack_25c = CONCAT31((int3)((uint)iVar3 >> 8),iVar3 != 3);
        FUN_004bad88(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,iStack_8,uVar2,uVar7,uVar8,uVar18);
        uVar2 = FUN_0049d5f4(iStack_8);
        FUN_004dfc44(param_1,auStack_23c,uVar2);
        FUN_004baa34(*(undefined4 *)PTR_DAT_0052bfbc,auStack_23c,0,1);
      }
      if (iStack_c < 200) {
        FUN_004dfd2c(param_1,0);
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004dfd2c(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dfd5c(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004dfd4c(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004dfde4(param_1,0);
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004dfde4(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dfe28(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004dfe18(param_1,uVar2);
      if (iStack_c < 200) {
        iVar3 = FUN_0049d5cc(iStack_8);
        FUN_004dfeb0(param_1,*(undefined4 *)(&DAT_0052a918 + iVar3 * 4));
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004dfeb0(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dfef4(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004dfee4(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004dffb0(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004e018c(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e01a4(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e01b4(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e01c4(param_1,uVar2);
      if (iStack_c < 200) {
        if (*(int *)(param_1 + 0x254) == 9) {
          uVar2 = FUN_004e01bc(param_1);
          FUN_004e01d4(param_1,uVar2);
          FUN_004e01c4(param_1,0);
        }
        else {
          FUN_004e01d4(param_1,0);
        }
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004e01d4(param_1,uVar2);
      }
      uVar2 = FUN_004d6c18();
      FUN_004e01e4(param_1,uVar2,puVar22);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x27c);
      FUN_004e01f8(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004e0238(param_1,uVar2,puVar23);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x284);
      FUN_004e024c(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004e028c(param_1,uVar2,puVar24);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x28c);
      FUN_004e02a0(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004e02e0(param_1,uVar2,puVar25);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x294);
      FUN_004e02f4(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004e0334(param_1,uVar2,puVar26);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x29c);
      FUN_004e0348(param_1,uVar2);
      uVar2 = FUN_004d6c18();
      FUN_004e0388(param_1,uVar2,puVar27);
      uVar2 = FUN_0049d668(iStack_8,param_1 + 0x2a4);
      FUN_004e039c(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004e03e4(param_1,0);
        *(undefined4 *)(param_1 + 0x2ac) = 0;
        FUN_004e03f8(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004e03e4(param_1,uVar2,puVar28);
        uVar2 = FUN_0049d668(iStack_8,param_1 + 0x2ac);
        FUN_004e03f8(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e0430(param_1,uVar2);
      if (iStack_c < 0x65) {
        FUN_004e04f4(param_1,0);
      }
      else {
        uVar2 = FUN_004d6c18();
        FUN_004e04f4(param_1,uVar2,puVar29);
      }
      if (iStack_c < 0x65) {
        FUN_004e0504(param_1,0);
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004e0504(param_1,uVar2);
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e0530(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e05f0(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004e0600(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e0610(param_1,uVar2);
      if (iStack_c < 200) {
        FUN_004e095c(param_1,0x10);
      }
      else {
        uVar2 = FUN_0049d5cc(iStack_8);
        FUN_004e095c(param_1,uVar2);
      }
      iVar3 = FUN_0049d5cc(iStack_8);
      if ((iStack_c < 0xcb) && (1 < iStack_38)) {
        iVar5 = FUN_004e0954(param_1);
        iVar6 = iStack_38;
        if (iVar5 < iStack_38) {
          iVar6 = FUN_004e0954(param_1);
        }
        iVar3 = (iVar3 * 0x12) / iVar6;
        if (0xff < iVar3) {
          iVar3 = 0xff;
        }
      }
      *(int *)(param_1 + 0x2f4) = iVar3;
      if (199 < iStack_c) {
        iVar3 = FUN_0049d5cc(iStack_8);
      }
      if ((iStack_c < 0xcb) && (iVar3 = iVar3 * 3, 0xff < iVar3)) {
        iVar3 = 0xff;
      }
      *(int *)(param_1 + 0x2f0) = iVar3;
      uVar2 = FUN_004e0978(param_1);
      FUN_004e0990(param_1,uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004e09d8(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004e09b8(param_1,uVar2);
      uVar2 = FUN_0049d620(iStack_8);
      FUN_004e09c8(param_1,uVar2);
      cStack_3a = '\0';
      if ((uStack_10 & 1) == 0) {
        cVar1 = FUN_0049d5f4(iStack_8);
        if (cVar1 != '\0') {
          FUN_004a48cc(*(undefined4 *)(*(int *)(param_1 + 0x25b4) + 0xc),iStack_8);
          if (0x65 < iStack_c) {
            FUN_004a4574(*(undefined4 *)(*(int *)(param_1 + 0x25b4) + 8),iStack_8);
          }
          (**(code **)(**(int **)(param_1 + 0x25b4) + 0xc))(*(int **)(param_1 + 0x25b4),iStack_8,0);
          cStack_3a = '\x01';
        }
      }
      else {
        cVar1 = FUN_0049d5f4(iStack_8);
        if (cVar1 != '\0') {
          FUN_0049d45c(iStack_8,iStack_18 + -9,0);
        }
      }
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004d1600(*(undefined4 *)(param_1 + 0x25b4),uVar2);
      uVar2 = FUN_0049d5cc(iStack_8);
      FUN_004d1618(*(undefined4 *)(param_1 + 0x25b4),uVar2);
      cVar1 = FUN_0049d5f4(iStack_8);
      if (cVar1 == '\0') {
        if (cStack_3a == '\0') {
          FUN_004d14cc(*(undefined4 *)(param_1 + 0x25b4),0);
        }
      }
      else {
        FUN_004d14cc(*(undefined4 *)(param_1 + 0x25b4),3);
      }
      FUN_0049dc8c(iStack_8,iStack_18,"preset");
      FUN_004de5e0(param_1,iStack_38);
      if (cStack_32 == '\0') {
        FUN_004de664(param_1,0);
      }
    }
    FUN_004be828(param_1,iStack_8,0x67 < iStack_20,(uStack_10 & 2) != 0);
    FUN_0049dc8c(iStack_8,uStack_1c,"Patch");
    uStack_11 = 1;
    FUN_004a454c(*(undefined4 *)(param_1 + 8));
    if (cStack_39 != '\0') {
      (**(code **)(param_1 + 0x58))(*(undefined4 *)(param_1 + 0x5c),param_1,0);
    }
  }
  puVar9 = puStack_254;
  *in_FS_OFFSET = uStack_25c;
  puStack_254 = &LAB_004d7bc9;
  puStack_258 = (undefined1 *)0x4d7bb9;
  FUN_004042a0(&uStack_244,2,puVar9);
  puStack_258 = (undefined1 *)0x4d7bc1;
  FUN_0040427c(&iStack_28);
  return;
}



