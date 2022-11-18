#version 460

layout(binding=0)uniform sampler2DArray samplerArray;

layout(location=0)in vec4 inColor;
layout(location=1)in vec3 inFragTextureCoords;
layout(location=2)in vec4 playerColor;
// Deprecated
layout(location=3)flat in int highlightPlayers;

layout(location=0)out vec4 outFragColor;

struct PlayerInfo{
    vec4 playerColor;
    vec4 playerObservableGrid;
};

struct ObjectData{
    mat4 modelMatrix;
    vec4 color;
    vec2 textureMultiply;
    int textureIndex;
    int objectType;
    int playerId;
    vec4 gridPosition;
};

layout(std140,binding=1)uniform EnvironmentData{
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec2 gridDims;
    int playerCount;
    int playerId;
    int globalVariableCount;
    int objectVariableCount;
    int globalObserverAvatarMode;
    int highlightPlayers;
}
environmentData;

layout(std430,binding=2)readonly buffer PlayerInfoBuffer{
    PlayerInfo variables[];
}
playerInfoBuffer;

layout(std430,binding=3)readonly buffer ObjectDataBuffer{
    uint size;
    ObjectData variables[];
}
objectDataBuffer;

void main()
{
    if(highlightPlayers==1){
        // Just multiply by the alpha channel of the object
        vec4 color=texture(samplerArray,inFragTextureCoords)*inColor;
        
        vec2 tex_dims=vec2(textureSize(samplerArray,0));
        
        vec2 pixel_size=2./tex_dims;
        
        vec4 colorU=texture(samplerArray,vec3(inFragTextureCoords.x,max(pixel_size.y,inFragTextureCoords.y-pixel_size.y),inFragTextureCoords.z));
        vec4 colorD=texture(samplerArray,vec3(inFragTextureCoords.x,min(tex_dims.y,inFragTextureCoords.y+pixel_size.y),inFragTextureCoords.z));
        vec4 colorL=texture(samplerArray,vec3(min(tex_dims.x,inFragTextureCoords.x+pixel_size.x),inFragTextureCoords.y,inFragTextureCoords.z));
        vec4 colorR=texture(samplerArray,vec3(max(0.,inFragTextureCoords.x-pixel_size.x),inFragTextureCoords.y,inFragTextureCoords.z));
        
        outFragColor=color;
        
        float thresh1=.7;
        float thresh2=.4;
        
        if(color.a<=thresh1&&(colorU.a>thresh2||colorD.a>thresh2||colorL.a>thresh2||colorR.a>thresh2)){
            outFragColor=playerColor;
        }
        
    }else{
        outFragColor=texture(samplerArray,inFragTextureCoords)*inColor;
    }
    
    if(environmentData.globalObserverAvatarMode==1) {
        outFragColor=outFragColor*0.6;
    }
    
}