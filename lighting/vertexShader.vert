#version 330 core
layout (location = 0) in vec3 vertexPos;
layout (location = 2) in vec3 normal;

out vec3 vertexNormal;
out vec3 worldPosition;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
gl_Position = projectionMatrix * viewMatrix *modelMatrix* vec4(vertexPos, 1.0);
vertexNormal = normal;
worldPosition = vec3(modelMatrix*vec4(vertexPos,1.0));
}