# Ribbon / Beam 파티클 텍스처

엔진의 SpriteShader, AlphaBlend, LinearClamp 설정에 맞춘 투명 RGBA PNG입니다.
두 파일 모두 2172 × 724이며, 가로(U)는 파티클 길이, 세로(V)는 폭입니다.

| 텍스처 | 형태 | 등록된 머티리얼 | 파티클 프리셋 |
| --- | --- | --- | --- |
| `RibbonFlow.png` | 밝은 머리에서 가늘게 흩어지는 에너지 잔상 | `RibbonFlowMaterial` | `Assets/Particles/RibbonFlow.particlesys` |
| `BeamArc.png` | 밝은 중심선과 갈라지는 전기 아크 | `BeamArcMaterial` | `Assets/Particles/BeamArc.particlesys` |

## 사용

1. 엔진을 빌드한 후 에디터를 다시 실행합니다. 시작 시 텍스처와 전용 머티리얼이 등록됩니다.
2. ParticleSystemComponent에 위 프리셋 중 하나를 지정합니다.
3. Ribbon은 액터를 이동시켜 잔상을 만듭니다. 이미지 왼쪽이 최신 머리이고 오른쪽이 오래된 꼬리입니다. 정지한 액터에서는 띠 길이가 생기지 않습니다.
4. Beam은 길이 8, 폭 0.4의 청록색 빔으로 시작합니다. Ribbon은 폭 0.3, 수명 1초, 초당 30개, 주황색입니다.
5. 색상 변경은 `UParticleModuleColor.StartColor`에서 합니다. 이 셰이더 경로는 머티리얼의 `BaseColor`를 사용하지 않습니다.

텍스처의 부드러운 알파에 여백과 끝부분 페이드가 포함되어 있습니다. 가로 반복 타일링용이 아니라 띠 전체에 한 장을 펴는 용도입니다.
현재 셰이더에는 UV 시간 스크롤이 없으므로 텍스처 자체는 정적입니다. Ribbon의 움직임은 액터 이동과 입자 수명에서 만들어집니다.

## 제작

내장 `image_gen` 도구의 투명 배경 모드로 생성했습니다. 생성된 PNG의 RGB와 알파를 그대로 보존했습니다.
정확한 생성 프롬프트는 `generation-prompts.json`에 있습니다. 프롬프트의 요청 크기는 1536 × 512였으며 도구가 반환한 실제 크기는 2172 × 724입니다.
생성 결과의 가장자리 알파 최댓값은 1/255 이하로, 엔진의 알파 컷오프 0.01보다 낮습니다.
