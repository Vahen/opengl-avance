#version 330

in vec2 vTexCoords;
in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;


uniform vec3 uPointLightPosition;
uniform vec3 uPointLightIntensity;

uniform vec3 uDirectionalLightDir;
uniform vec3 uDirectionalLightIntensity;

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

vec3 blinnPhong(vec3 wi){
	vec3 _uKd = uKd * texture(uKdTexture, vTexCoords).xyz;
	vec3 _uKa = uKa * texture(uKaTexture, vTexCoords).xyz;
	vec3 _uKs = uKs * texture(uKsTexture, vTexCoords).xyz;
	float _uSn = uShininess * texture(uSnTexture, vTexCoords).x;

	vec3 normal = normalize(vViewSpaceNormal);

    vec3 w0 = normalize(-vViewSpacePosition);
    vec3 halfVector = (wi + w0)/2;


    vec3 diffuse = _uKd *clamp(dot(wi,normal),0,1);
    // todo -> virer le max
    vec3 specular = uKs*pow(clamp(dot(halfVector, normal),0,1), max(_uSn,1));
    vec3 color = diffuse + specular;

    return color;
}

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