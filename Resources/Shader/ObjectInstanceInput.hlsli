#ifndef HITORI_OBJECT_INSTANCE_INPUT_INCLUDED
#define HITORI_OBJECT_INSTANCE_INPUT_INCLUDED

#ifndef USE_INSTANCING
#define USE_INSTANCING 0
#endif

struct FObjectInstanceInput {
    float4 Model0 : INSTANCE_MODEL0;
    float4 Model1 : INSTANCE_MODEL1;
    float4 Model2 : INSTANCE_MODEL2;
    float4 Model3 : INSTANCE_MODEL3;
    float4 Normal0 : INSTANCE_NORMAL0;
    float4 Normal1 : INSTANCE_NORMAL1;
    float4 Normal2 : INSTANCE_NORMAL2;
    float4 Normal3 : INSTANCE_NORMAL3;
    float4 Color : INSTANCE_COLOR;
    uint ObjectMask : INSTANCE_MASK;
};

#endif
