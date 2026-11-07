from pathlib import Path
p=Path('controls-src/test_controls.cpp');s=p.read_text().replace('== -20','==20').replace('==-20','==20')
s=s.replace('dy=-10000;Third(camera);','dy=10000;Third(camera);').replace('dy=10000;Third(camera);assert(*(short*)(camera+0x2e)==-512)','dy=-10000;Third(camera);assert(*(short*)(camera+0x2e)==-512)')
s=s.replace('SetAction(0x400);assert', 'SetAction(0x8000);assert(mem<unsigned char>(0x529a00+DIK_LCONTROL)==0x80);\n SetAction(0x400);assert')
s=s.replace('for(int n=0;n<30;n++)AimMovement(player,movementParameters);', '''for(int n=0;n<30;n++){
  // Reproduce the native visor friction before every physics update.
  *(int*)(player+0x68)=Approach(*(int*)(player+0x68),0,128);
  *(int*)(player+0x70)=Approach(*(int*)(player+0x70),0,48);
  AimMovement(player,movementParameters);
 }''')
s=s.replace(' puts("PASS:', ''' assert(ScaleVolume(60,0)==0&&ScaleVolume(60,50)==30&&ScaleVolume(60,100)==60);
 assert(ScaleVolume(120,25)==30);
 puts("PASS:''')
p.write_text(s)
