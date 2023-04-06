#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform samplerCube skyboxFOM;
uniform float coef;
void main()
{
    FragColor = (1.0 - coef) * texture(skybox, TexCoords) + coef * texture(skyboxFOM, TexCoords);
}