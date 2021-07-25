#version 330 core
in vec3 vertexColor;
in vec3 vertexNormal;
in vec3 worldPosition;

out vec4 fragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPosition;

uniform float ambientScale;

void main()
{
vec3 ambience = lightColor*ambientScale;

vec3 unitNormal = normalize(vertexNormal);
vec3 lightDirection = normalize(lightPosition - worldPosition);

float angle = max(dot(vertexNormal, lightPosition), 0.0);
vec3 diffuse = lightColor*angle;
vec3 finalColor = (ambience + diffuse)*objectColor;
// add ambient color
fragColor = vec4(finalColor, 1.0);
}