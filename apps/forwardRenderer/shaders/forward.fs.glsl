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

out vec3 fColor;

uniform sampler2D uKaTexture;
uniform sampler2D uKdTexture;
uniform sampler2D uKsTexture;
uniform sampler2D uSnTexture;


vec3 phong(){
	float distToPointLight = length(uPointLightPosition - vViewSpacePosition);

	vec3 dirToPointLight = (uPointLightPosition - vViewSpacePosition) / distToPointLight;

	return uKd * (uDirectionalLightIntensity * max(0.0, dot(vViewSpaceNormal, uDirectionalLightDir)) + uPointLightIntensity * max(0.0, dot(vViewSpaceNormal, dirToPointLight)) / (distToPointLight * distToPointLight));
}

vec3 blinnPhong(vec3 wi){
	vec3 _uKd = uKd * texture(uKdTexture, vTexCoords).xyz;
	vec3 _uKa = uKa * texture(uKaTexture, vTexCoords).xyz;
	vec3 _uKs = uKs * texture(uKsTexture, vTexCoords).xyz;
	float _uSn = uShininess * texture(uSnTexture, vTexCoords).x;

	vec3 normal = normalize(vViewSpaceNormal);

    vec3 w0 = normalize(-vViewSpacePosition);
    vec3 halfVector = (wi + w0)/2;


    vec3 diffuse = _uKd *clamp(dot(wi,normal),0,1);
    vec3 specular = uKs*pow(clamp(dot(halfVector, normal),0,1), max(_uSn,1));
    vec3 color = diffuse + specular;

    return color;
}

void main(){

	vec3 widir = normalize(uDirectionalLightDir);
	vec3 colorDir = blinnPhong(widir)*uDirectionalLightIntensity;
	vec3 wipoint = normalize(uPointLightPosition-vViewSpacePosition);
	float d = distance(uPointLightPosition, vViewSpacePosition);

	vec3 colorPoint = blinnPhong(wipoint)*(uPointLightIntensity / (d*d));

	fColor = texture(uKaTexture, vTexCoords).xyz+ colorDir+ colorPoint;
	fColor = colorPoint;


	//fColor = normalize(vViewSpaceNormal);
}