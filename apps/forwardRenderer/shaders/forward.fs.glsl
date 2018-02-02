#version 330

in vec2 vTexCoords;
in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;

out vec3 fColor;

uniform sampler2D uSampler;

void main()
{
   fColor = normalize(vViewSpaceNormal);
}