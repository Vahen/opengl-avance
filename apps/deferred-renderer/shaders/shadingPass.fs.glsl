#version 330

uniform vec3 uPointLightPosition;
uniform vec3 uPointLightIntensity;

uniform vec3 uDirectionalLightDir;
uniform vec3 uDirectionalLightIntensity;

out vec3 fColor;

uniform sampler2D uGPosition;
uniform sampler2D uGNormal;
uniform sampler2D uGAmbient;
uniform sampler2D uGDiffuse;
uniform sampler2D uGlossyShininess;

//////////////////////////////////////////////////////////
// offres stages http://fairytool.com/internships_fr.php//
//////////////////////////////////////////////////////////


vec3 blinnPhong(vec3 wi,vec3 normal,vec3 position,vec3 diffuse,vec3 glossyShininess){

    vec3 w0 = normalize(-position);
    vec3 halfVector = (wi + w0)/2;

    vec3 specular = glossyShininess;

    vec3 color = diffuse + specular;

    return color;
}

void main(){

	vec3 position = vec3(texelFetch(uGPosition, ivec2(gl_FragCoord.xy), 0));
	vec3 normal = vec3(texelFetch(uGNormal, ivec2(gl_FragCoord.xy), 0));
	vec3 ambient = vec3(texelFetch(uGAmbient, ivec2(gl_FragCoord.xy), 0));
	vec3 diffuse = vec3(texelFetch(uGDiffuse, ivec2(gl_FragCoord.xy), 0));
	vec3 glossyShininess = vec3(texelFetch(uGlossyShininess, ivec2(gl_FragCoord.xy), 0));

	vec3 widir = normalize(uDirectionalLightDir);
	vec3 colorDir = blinnPhong(widir,normal,position,diffuse,glossyShininess)*uDirectionalLightIntensity;
	vec3 wipoint = normalize(uPointLightPosition-position);
	float d = distance(uPointLightPosition, position);

	vec3 colorPoint = blinnPhong(wipoint,normal,position,diffuse,glossyShininess)*(uPointLightIntensity / (d*d));

	fColor = colorPoint;

}