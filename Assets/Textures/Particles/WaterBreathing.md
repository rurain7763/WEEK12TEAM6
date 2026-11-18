# 물의 호흡 Ribbon

- 텍스처: `WaterBreathing_Ribbon.png`, 2172 × 724, RGBA PNG.
- 프리셋: `Assets/Particles/WaterBreathing.particlesys`.
- 푸른 물살, 흰 포말, 청록색으로 가늘게 사라지는 꼬리. 가로 U=0이 최신 머리, U=1이 오래된 꼬리입니다.
- 띠 전체에 텍스처 한 장을 펼치는 용도이며 반복 타일링용이 아닙니다.

## 적용

ParticleSystemComponent에 WaterBreathing 프리셋을 지정하고 액터를 이동시키면 잔상이 생깁니다. 정지한 액터에서는 띠 길이가 생기지 않습니다.
프리셋은 폭 0.7, 수명 0.45초, 초당 90개로 시작합니다. 검 끝에 연결하려면 프로젝트에서 사용하는 본/소켓 위치 모듈을 별도로 지정하세요.
텍스처에 파랑과 흰색이 포함되어 있으므로 StartColor와 머티리얼 BaseColor를 흰색 (1, 1, 1, 1)으로 유지하면 원색이 보존됩니다.
MaterialInstance가 BeamMaterial을 기반으로 텍스처를 참조하므로 새 머티리얼 등록이나 소스 변경은 필요하지 않습니다.

## 제작 및 확인

내장 image_gen의 투명 배경 모드로 제작했으며 원본 RGB와 알파를 보존했습니다. 정확한 프롬프트는 `WaterBreathing-generation.json`에 있습니다.
이미지의 실제 RGBA 형식과 가장자리 알파(최댓값 1/255), 프리셋 JSON과 참조 경로를 확인했습니다. 엔진 실행 화면 확인은 수행하지 않았습니다.
