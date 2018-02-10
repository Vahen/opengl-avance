#version 330

in vec2 vTexCoords;
in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;

uniform vec3 uKa;
uniform vec3 uKd;
uniform vec3 uKs;
uniform float uShininess;

uniform sampler2D uKaTexture;
uniform sampler2D uKdTexture;
uniform sampler2D uKsTexture;
uniform sampler2D uSnTexture;

layout(location = 0) out vec3 fPosition;
layout(location = 1) out vec3 fNormal;
layout(location = 2) out vec3 fAmbient;
layout(location = 3) out vec3 fDiffuse;
layout(location = 4) out vec4 fGlossyShininess;

void main(){

    fPosition = normalize(vViewSpacePosition);
    fNormal = normalize(vViewSpaceNormal);

    vec3 _uKa = uKa * texture(uKaTexture, vTexCoords).xyz;
    vec3 _uKd = uKd * texture(uKdTexture, vTexCoords).xyz;
    vec3 _uKs = uKs * texture(uKsTexture, vTexCoords).xyz;
    float _uSn = uShininess * texture(uSnTexture, vTexCoords).x;

    fAmbient = _uKa;
    fDiffuse = _uKd;
    fGlossyShininess = vec4(_uKs,_uSn);
}